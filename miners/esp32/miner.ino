#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <mbedtls/sha256.h>
#include <mbedtls/pk.h>
#include <mbedtls/base64.h>
#include <mbedtls/version.h>
#include <esp_system.h>


// SPDX-License-Identifier: Apache-2.0
/*
 * MINI ESP32 Reference Miner
 *
 * Copyright 2026 Mini Miners Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Project: https://miniminers.net
 * Source:  https://github.com/MiniMinersProject/network
 *
 * Public reference miner: basic functionality needed to configure an ESP32,
 * connect to the MINI mining network, authenticate, perform MiniPOW1, submit
 * shares, reconnect, and provide simple status through the onboard LED.
 *
 * This reference build intentionally does NOT include the proprietary
 * Mini Miners OTA/update system, commercial provisioning/device-management
 * features, or OLED/display support.
 */

// --------------------------------------------------
// BUILD / BURN-IN SETTINGS
// Select ONE board profile before compiling.
// --------------------------------------------------
enum MiniMinerBoardProfile {
    BOARD_DEVKIT,
    BOARD_NODEMCU_32S
};

const MiniMinerBoardProfile BOARD_PROFILE = BOARD_DEVKIT;

// Optional local defaults. For a public reference build, leave all four blank.
// On a clean first flash these values seed setup only. Once settings are saved,
// NVS values take priority.
const char* FACTORY_MINER_NAME = "";
const char* FACTORY_WIFI_SSID  = "";
const char* FACTORY_WIFI_PASS  = "";
const char* FACTORY_WALLET     = "";

// Board profile facts used by this reference build.
// DOIT-style DevKit V1: onboard blue LED is GPIO2, active HIGH.
// NodeMCU-32S: onboard LED is GPIO2, active LOW.
constexpr int STATUS_LED_PIN = 2;
constexpr bool STATUS_LED_ACTIVE_HIGH =
    (BOARD_PROFILE == BOARD_NODEMCU_32S) ? false : true;

// --------------------------------------------------
// HUMAN-VISIBLE STATUS LED PATTERNS
// Normal mining: OFF
// Accepted share: solid ON for 1 second
// Setup/BOOT mode: 2 sec ON / 1 sec OFF, repeating
// Wi-Fi/pool problem: 3 x (1 sec ON / 1 sec OFF), then 3 sec OFF
// --------------------------------------------------
const unsigned long LED_SHARE_MS = 1000UL;
const unsigned long LED_SETUP_ON_MS = 2000UL;
const unsigned long LED_SETUP_OFF_MS = 1000UL;
const unsigned long LED_CONNECTION_BLINK_MS = 1000UL;
const unsigned long LED_CONNECTION_PAUSE_MS = 3000UL;

enum StatusLedMode { LED_NORMAL, LED_SETUP, LED_CONNECTION };
StatusLedMode statusLedMode = LED_NORMAL;
unsigned long statusLedShareUntil = 0;

void writeStatusLed(bool on) {
    digitalWrite(STATUS_LED_PIN, (on == STATUS_LED_ACTIVE_HIGH) ? HIGH : LOW);
}

void flashAcceptedShare() {
    statusLedShareUntil = millis() + LED_SHARE_MS;
    writeStatusLed(true);  // Immediate visible acknowledgement.
}

void serviceStatusLed() {
    unsigned long now = millis();


    if (statusLedMode == LED_SETUP) {
        const unsigned long period = LED_SETUP_ON_MS + LED_SETUP_OFF_MS;
        writeStatusLed((now % period) < LED_SETUP_ON_MS);
        return;
    }

    if (statusLedMode == LED_CONNECTION) {
        const unsigned long cycle = (5UL * LED_CONNECTION_BLINK_MS) + LED_CONNECTION_PAUSE_MS;
        unsigned long phase = now % cycle;
        writeStatusLed(
            phase < LED_CONNECTION_BLINK_MS ||
            (phase >= 2UL * LED_CONNECTION_BLINK_MS && phase < 3UL * LED_CONNECTION_BLINK_MS) ||
            (phase >= 4UL * LED_CONNECTION_BLINK_MS && phase < 5UL * LED_CONNECTION_BLINK_MS)
        );
        return;
    }

    if (statusLedShareUntil && (long)(statusLedShareUntil - now) > 0) {
        writeStatusLed(true);
        return;
    }

    statusLedShareUntil = 0;
    writeStatusLed(false);
}

// --------------------------------------------------
// Wi-Fi / manual BOOT-button setup
// BOOT = configuration mode. RESET = normal restart/mining.
// Normal mining never exposes a setup hotspot or settings server.
// --------------------------------------------------
const int SETUP_BOOT_PIN = 0;
const char* SETUP_AP_PREFIX = "MINI-";  // AP name gets a unique MAC suffix; setup URL is always 192.168.4.1
const unsigned long WIFI_CONNECT_TIMEOUT_MS = 20000;

Preferences preferences;
WebServer setupServer(80);

String savedSSID;
String savedPassword;
String minerName;
String walletAddress;
bool setupMode = false;

// --------------------------------------------------
// Persistent cryptographic miner identity
//
// ECDSA P-256 key is generated once and saved in NVS.
// Only the public key leaves the miner.
// --------------------------------------------------

mbedtls_pk_context identityKey;
bool identityReady = false;
String identityPublicKeyB64;
String identityFingerprint;

int miniMinerRandom(void* context, unsigned char* output, size_t length) {
    (void)context;
    esp_fill_random(output, length);
    return 0;
}


// --------------------------------------------------
// Mini Miners server
// --------------------------------------------------

const char* FACTORY_POOL_HOST = "144.202.27.176";
const uint16_t FACTORY_POOL_PORT = 2811;
// GitHub-hosted pool discovery. Replace this one constant with the permanent
// raw.githubusercontent.com URL before release. Until then the miner safely
// uses its cached list or the embedded factory fallback below.
const char* POOL_DISCOVERY_URL =
    "https://raw.githubusercontent.com/MiniMinersProject/network/main/pools.json";
const unsigned long POOL_DISCOVERY_TIMEOUT_MS = 5000UL;
const unsigned long POOL_DISCOVERY_REFRESH_MS = 6UL * 60UL * 60UL * 1000UL;
constexpr uint8_t MAX_POOLS = 5;
String poolHosts[MAX_POOLS];
uint16_t poolPorts[MAX_POOLS];
uint8_t poolCount = 0;
uint8_t currentPoolIndex = 0;
unsigned long lastPoolDiscoveryCheck = 0;
String poolHost = FACTORY_POOL_HOST;
uint16_t poolPort = FACTORY_POOL_PORT;

const char* MINER_TYPE = "ESP32";

const char* PROTOCOL_VERSION = "MINIMINERS/0.3";


// --------------------------------------------------
// Public reference build version
// --------------------------------------------------

const char* FIRMWARE_VERSION = "1.0.0-reference";


// --------------------------------------------------
// MiniPOW1
// --------------------------------------------------

constexpr size_t SCRATCHPAD_SIZE = 64 * 1024;
constexpr size_t BLOCK_SIZE = 32;
constexpr size_t BLOCK_COUNT =
    SCRATCHPAD_SIZE / BLOCK_SIZE;

constexpr uint32_t MIX_ROUNDS = 512;

static uint8_t scratchpad[SCRATCHPAD_SIZE];


// --------------------------------------------------
// MiniPOW1 test vector
// --------------------------------------------------

const char* TEST_CHALLENGE =
    "00112233445566778899aabbccddeeff";

const uint64_t TEST_NONCE = 42;

const char* TEST_RESULT =
    "f67043c783d01c3104f322c76de5d4f0"
    "fbbac377c724b48546c238c06f505657";


// --------------------------------------------------
// Networking
// --------------------------------------------------


WiFiClient client;

// --------------------------------------------------
// Miner runtime statistics / state
// --------------------------------------------------

uint32_t acceptedShares = 0;
uint32_t rejectedShares = 0;
uint32_t completedJobs = 0;
uint32_t reconnectCount = 0;
float currentSpeed = 0.0f;
float lastShareSeconds = 0.0f;
float sessionEarnedMini = 0.0f;
uint32_t currentDifficulty = 0;
uint32_t lastAttempts = 0;

bool wifiPowerSaving = false;

String currentStatus = "BOOTING";
String lastPoolResult = "";

// --------------------------------------------------
// Mining recovery watchdog
// --------------------------------------------------
const unsigned long MINING_STALL_REBOOT_MS = 5UL * 60UL * 1000UL;
bool miningWatchdogArmed=false;
unsigned long lastMiningProgressMs=0;
void armMiningWatchdog(){ miningWatchdogArmed=true; lastMiningProgressMs=millis(); }
void noteMiningProgress(){ if(miningWatchdogArmed) lastMiningProgressMs=millis(); }
void serviceMiningWatchdog(){ if(miningWatchdogArmed && millis()-lastMiningProgressMs>=MINING_STALL_REBOOT_MS){ Serial.println("MINING WATCHDOG REBOOT: no mining progress for 5 minutes"); Serial.flush(); delay(250); ESP.restart(); } }

// --------------------------------------------------
// First-boot configuration portal
// --------------------------------------------------

bool isValidMinerName(const String& name) {
    // Miner name is only a changeable human-readable label. It is NOT a hostname.
    // The wire protocol uses '|' as a field separator, so disallow that and controls.
    if (name.length() < 1 || name.length() > 24) return false;
    for (size_t i = 0; i < name.length(); i++) {
        unsigned char c = (unsigned char)name[i];
        if (c < 32 || c == 127 || c == '|') return false;
    }
    return true;
}


String htmlEscape(const String& value) {
    String out;
    out.reserve(value.length() + 8);
    for (size_t i = 0; i < value.length(); i++) {
        char c = value[i];
        if (c == '&') out += "&amp;";
        else if (c == '<') out += "&lt;";
        else if (c == '>') out += "&gt;";
        else if (c == '"') out += "&quot;";
        else if (c == '\'') out += "&#39;";
        else out += c;
    }
    return out;
}

String setupAccessPointName() {
    // Do not tie setup access to the miner name. The miner name can change.
    // A short MAC suffix keeps multiple unconfigured miners distinguishable.
    uint64_t mac = ESP.getEfuseMac();
    char suffix[7];
    snprintf(suffix, sizeof(suffix), "%06llX", (unsigned long long)(mac & 0xFFFFFFULL));
    return String(SETUP_AP_PREFIX) + suffix;
}

String buildSetupPage() {
    int networkCount = WiFi.scanNetworks(false, true);

    String page = R"HTML(
<!doctype html><html><head><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Mini Miners Setup</title>
<style>
body{font-family:Arial,sans-serif;background:#111;color:#eee;margin:0;padding:20px}
.card{max-width:520px;margin:auto;background:#1d1d1d;padding:24px;border-radius:14px}
h1{margin-top:0}label{display:block;margin:16px 0 6px}select,input{width:100%;box-sizing:border-box;padding:12px;border-radius:8px;border:1px solid #555;background:#fff;color:#111;font-size:16px}
button{width:100%;margin-top:22px;padding:14px;border:0;border-radius:8px;font-size:17px;font-weight:bold;cursor:pointer}
.small{color:#bbb;font-size:13px;line-height:1.4}
</style></head><body><div class="card">
<h1>Mini Miners Setup</h1>
<p>Connect this miner to Wi-Fi and enter this miner's settings. This setup page is always reached at <strong>192.168.4.1</strong> while connected to the miner Wi-Fi.</p>
<form method="POST" action="/save">
<label>Wi-Fi network</label><select name="ssid" required>
)HTML";

    // Always show the currently saved network first. This makes setup useful
    // even when the miner is temporarily out of range of that network.
    if (savedSSID.length()) {
        page += "<option value=\"" + htmlEscape(savedSSID) + "\" selected>";
        page += htmlEscape(savedSSID) + " [currently saved]</option>";
    }

    if (networkCount <= 0 && savedSSID.length() == 0) {
        page += "<option value=\"\">No networks found - rescan below</option>";
    } else {
        for (int i = 0; i < networkCount; i++) {
            String ssid = WiFi.SSID(i);
            if (ssid.length() == 0 || ssid == savedSSID) continue;
            page += "<option value=\"" + htmlEscape(ssid) + "\">";
            page += htmlEscape(ssid) + " (" + String(WiFi.RSSI(i)) + " dBm)";
            if (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) page += " [open]";
            page += "</option>";
        }
    }

    page += R"HTML(
</select>
<label>Wi-Fi password</label><input type="password" name="password" maxlength="63" autocomplete="new-password" placeholder="Leave blank to keep current password"><p class="small">Leave blank to keep the saved password when keeping the same Wi-Fi network.</p>
<label>Mini Miners wallet</label><input type="text" name="wallet" maxlength="80" value=")HTML";
    page += htmlEscape(walletAddress);
    page += R"HTML(" required>
<p style="color:#ff3b30;font-weight:800">IMPORTANT: Rewards are paid to the wallet shown here. Make sure it is your registered wallet.</p>
<p class="small">This wallet must already be registered before mining can begin.</p>
<p class="small"><strong>No account yet?</strong> Create/register your Mini Miners account on miniminers.org first, then return here and enter its wallet address.</p>
<label>Miner name</label><input type="text" name="miner" maxlength="24" value=")HTML";
    page += htmlEscape(minerName);
    page += R"HTML(" required>
<p class="small">A changeable name for this individual miner, such as DevKit-1 or Solar-Miner-2. It is only a label; it is not a web address.</p>
<label>Power Saving</label>
<label style="display:flex;gap:10px;align-items:center">
<input style="width:auto" type="checkbox" name="wifi_sleep" value="1" )HTML";
    if (wifiPowerSaving) page += "checked";
    page += R"HTML(>
<span>Enable Wi-Fi power saving</span>
</label>
<p class="small">Uses slightly less power. Leave this OFF if you want maximum Wi-Fi reliability.</p>
<button type="submit">Save &amp; Start Mining</button>
</form>
<form method="GET" action="/" style="margin-top:10px"><button type="submit">Rescan Wi-Fi</button></form>
</div></body></html>)HTML";

    WiFi.scanDelete();
    return page;
}

void handleSetupRoot() {
    setupServer.send(200, "text/html", buildSetupPage());
}

void handleSetupSave() {
    String ssid = setupServer.arg("ssid");
    String password = setupServer.arg("password");
    String requestedWallet = setupServer.arg("wallet");
    String requestedMinerName = setupServer.arg("miner");
    bool requestedWifiPowerSaving = setupServer.hasArg("wifi_sleep");
    ssid.trim();
    requestedWallet.trim();
    requestedMinerName.trim();

    if (!isValidMinerName(requestedMinerName)) {
        setupServer.send(400, "text/plain", "Miner name must be 1-24 characters and cannot contain control characters or |.");
        return;
    }

    if (ssid.length() == 0 || requestedWallet.length() == 0 || requestedMinerName.length() == 0) {
        setupServer.send(400, "text/plain", "Wi-Fi network, registered wallet address, and miner name are required.");
        return;
    }

    if (requestedWallet.length() > 80) requestedWallet = requestedWallet.substring(0, 80);
    
    preferences.begin("miniminer", false);
    preferences.putString("ssid", ssid);
    // Blank password means keep the existing password when the SSID is unchanged.
    // If a different SSID is selected, blank intentionally means an open network.
    if (password.length() > 0 || ssid != savedSSID) {
        preferences.putString("pass", password);
    }
    preferences.putString("wallet", requestedWallet);
    preferences.putString("name", requestedMinerName);
    preferences.putBool("wifi_sleep", requestedWifiPowerSaving);
    preferences.end();

    setupServer.send(200, "text/html",
        "<!doctype html><html><body style='font-family:Arial;text-align:center;padding:40px'>"
        "<h1>Saved!</h1><p>Your Mini Miner is restarting and will begin mining.</p>"
        "<p>You can close this page.</p></body></html>");

    delay(1500);
    ESP.restart();
}

void startSetupMode() {
    setupMode = true;
    statusLedMode = LED_SETUP;
    serviceStatusLed();
    client.stop();

    // Start from a known Wi-Fi state. Do not depend on Arduino/ESP32 defaults.
    WiFi.mode(WIFI_OFF);
    delay(150);
    WiFi.mode(WIFI_AP_STA);  // STA is kept only so the setup page can scan nearby Wi-Fi.
    delay(150);

    const IPAddress apIP(192, 168, 4, 1);
    const IPAddress gateway(192, 168, 4, 1);
    const IPAddress subnet(255, 255, 255, 0);
    WiFi.softAPConfig(apIP, gateway, subnet);

    String apName = setupAccessPointName();
    if (!WiFi.softAP(apName.c_str())) {
        Serial.println("ERROR: setup Wi-Fi AP failed to start.");
        return;
    }
    delay(250);

    // Setup is intentionally local to the miner AP.
    // 192.168.4.1 is always the reliable setup address.
    Serial.println("Setup address: http://192.168.4.1");

    setupServer.on("/", HTTP_GET, handleSetupRoot);
    setupServer.on("/save", HTTP_POST, handleSetupSave);

    // No mDNS and no captive-portal dependency. The setup address is always 192.168.4.1.
    setupServer.onNotFound(handleSetupRoot);
    setupServer.begin();

    Serial.println();
    Serial.println("Mini Miners setup mode");
    Serial.print("Connect to Wi-Fi: ");
    Serial.println(apName);
    Serial.println("Open: http://192.168.4.1");
    Serial.print("AP IP: http://");
    Serial.println(WiFi.softAPIP());
}

void runSetupMode() {
    serviceStatusLed();
    setupServer.handleClient();
    delay(2);
}

void loadSavedConfiguration() {
    // Persistent-configuration rule: configured values come ONLY from persistent NVS.
    // Firmware updates must never replace saved Wi-Fi, wallet, miner name, or identity.
    preferences.begin("miniminer", true);
    savedSSID = preferences.getString("ssid", FACTORY_WIFI_SSID);
    savedPassword = preferences.getString("pass", FACTORY_WIFI_PASS);
    walletAddress = preferences.getString("wallet", FACTORY_WALLET);
    minerName = preferences.getString("name", FACTORY_MINER_NAME);
    wifiPowerSaving = preferences.getBool("wifi_sleep", false);
    preferences.end();

    savedSSID.trim();
    walletAddress.trim();
    minerName.trim();
}

// BOOT must remain responsive even while Wi-Fi is unavailable.
// A normal press enters setup mode; no long press is required.
void serviceSetupButton();

bool connectSavedWiFi() {
    if (savedSSID.length() == 0) return false;
    statusLedMode = LED_CONNECTION;
    Serial.print("Connecting to Wi-Fi: "); Serial.println(savedSSID);
    WiFi.disconnect();
    delay(100);
    WiFi.mode(WIFI_STA);
    WiFi.begin(savedSSID.c_str(), savedPassword.c_str());

    unsigned long started = millis();
    unsigned long lastDot = 0;
    while (WiFi.status() != WL_CONNECTED && millis() - started < WIFI_CONNECT_TIMEOUT_MS) {
        serviceSetupButton();
        if (setupMode) return false;

        if (millis() - lastDot >= 500) {
            Serial.print(".");
            lastDot = millis();
        }
        delay(20);
    }

    Serial.println();
    if (setupMode) return false;
    if (WiFi.status() != WL_CONNECTED) return false;
    WiFi.setSleep(wifiPowerSaving);
    Serial.print("Wi-Fi connected. IP: "); Serial.println(WiFi.localIP());
    statusLedMode = LED_NORMAL;
    serviceStatusLed();
    return true;
}


// --------------------------------------------------
// Base64 helper
// --------------------------------------------------

String base64Encode(const uint8_t* data, size_t length) {
    size_t outputLength = 0;
    size_t bufferSize = ((length + 2) / 3) * 4 + 8;
    unsigned char* output = new unsigned char[bufferSize];

    if (output == nullptr) return "";

    int result = mbedtls_base64_encode(
        output,
        bufferSize,
        &outputLength,
        data,
        length
    );

    if (result != 0) {
        delete[] output;
        return "";
    }

    output[outputLength] = 0;
    String encoded = String((char*)output);
    delete[] output;
    return encoded;
}

String sha256Hex(const uint8_t* data, size_t length) {
    uint8_t digest[32];
    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts(&ctx, 0);
    mbedtls_sha256_update(&ctx, data, length);
    mbedtls_sha256_finish(&ctx, digest);
    mbedtls_sha256_free(&ctx);

    static const char hexChars[] = "0123456789abcdef";
    String out;
    out.reserve(64);
    for (size_t i = 0; i < sizeof(digest); i++) {
        out += hexChars[(digest[i] >> 4) & 0x0F];
        out += hexChars[digest[i] & 0x0F];
    }
    return out;
}

bool buildIdentityPublicInfo() {
    unsigned char publicPem[768];
    memset(publicPem, 0, sizeof(publicPem));

    if (mbedtls_pk_write_pubkey_pem(
            &identityKey,
            publicPem,
            sizeof(publicPem)
        ) != 0) {
        Serial.println("Failed to export identity public key.");
        return false;
    }

    size_t publicLength = strlen((char*)publicPem);
    identityPublicKeyB64 = base64Encode(publicPem, publicLength);
    identityFingerprint = sha256Hex(publicPem, publicLength);

    return identityPublicKeyB64.length() > 0 && identityFingerprint.length() == 64;
}

bool loadOrCreateIdentity() {
    mbedtls_pk_init(&identityKey);

    preferences.begin("miniminer", false);
    String storedPrivateKey = preferences.getString("id_key", "");

    if (storedPrivateKey.length() > 0) {
        int parseResult;

#if MBEDTLS_VERSION_NUMBER >= 0x03000000
        parseResult = mbedtls_pk_parse_key(
            &identityKey,
            (const unsigned char*)storedPrivateKey.c_str(),
            storedPrivateKey.length() + 1,
            nullptr,
            0,
            miniMinerRandom,
            nullptr
        );
#else
        parseResult = mbedtls_pk_parse_key(
            &identityKey,
            (const unsigned char*)storedPrivateKey.c_str(),
            storedPrivateKey.length() + 1,
            nullptr,
            0
        );
#endif

        if (parseResult == 0) {
            preferences.end();
            identityReady = buildIdentityPublicInfo();
            if (identityReady) {
                Serial.println("Existing persistent miner identity loaded.");
                Serial.print("Identity fingerprint: ");
                Serial.println(identityFingerprint);
            }
            return identityReady;
        }

        Serial.println("Stored identity could not be loaded; generating a replacement.");
        mbedtls_pk_free(&identityKey);
        mbedtls_pk_init(&identityKey);
    }

    const mbedtls_pk_info_t* info = mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY);
    if (info == nullptr || mbedtls_pk_setup(&identityKey, info) != 0) {
        preferences.end();
        Serial.println("Identity key setup failed.");
        return false;
    }

    mbedtls_ecp_keypair* ec = mbedtls_pk_ec(identityKey);

    if (mbedtls_ecp_gen_key(
            MBEDTLS_ECP_DP_SECP256R1,
            ec,
            miniMinerRandom,
            nullptr
        ) != 0) {
        preferences.end();
        Serial.println("Identity key generation failed.");
        return false;
    }

    unsigned char privatePem[1024];
    memset(privatePem, 0, sizeof(privatePem));

    if (mbedtls_pk_write_key_pem(
            &identityKey,
            privatePem,
            sizeof(privatePem)
        ) != 0) {
        preferences.end();
        Serial.println("Identity private-key export failed.");
        return false;
    }

    preferences.putString("id_key", (char*)privatePem);
    preferences.end();

    identityReady = buildIdentityPublicInfo();

    if (identityReady) {
        Serial.println("New persistent miner identity created.");
        Serial.print("Identity fingerprint: ");
        Serial.println(identityFingerprint);
    }

    return identityReady;
}

String signIdentityChallenge(const String& challenge) {
    if (!identityReady) return "";

    uint8_t hash[32];
    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts(&ctx, 0);
    mbedtls_sha256_update(
        &ctx,
        (const unsigned char*)challenge.c_str(),
        challenge.length()
    );
    mbedtls_sha256_finish(&ctx, hash);
    mbedtls_sha256_free(&ctx);

    unsigned char signature[160];
    size_t signatureLength = 0;

    int result;

#if MBEDTLS_VERSION_NUMBER >= 0x03000000
    result = mbedtls_pk_sign(
        &identityKey,
        MBEDTLS_MD_SHA256,
        hash,
        sizeof(hash),
        signature,
        sizeof(signature),
        &signatureLength,
        miniMinerRandom,
        nullptr
    );
#else
    result = mbedtls_pk_sign(
        &identityKey,
        MBEDTLS_MD_SHA256,
        hash,
        sizeof(hash),
        signature,
        &signatureLength,
        miniMinerRandom,
        nullptr
    );
#endif

    if (result != 0) {
        Serial.println("Identity challenge signing failed.");
        return "";
    }

    return base64Encode(signature, signatureLength);
}


// --------------------------------------------------
// SHA-256 helper
// --------------------------------------------------

void sha256Parts(
    const uint8_t* part1,
    size_t len1,
    const uint8_t* part2,
    size_t len2,
    const uint8_t* part3,
    size_t len3,
    uint8_t output[32]
) {
    mbedtls_sha256_context ctx;

    mbedtls_sha256_init(&ctx);

    mbedtls_sha256_starts(
        &ctx,
        0
    );

    if (part1 != nullptr && len1 > 0) {
        mbedtls_sha256_update(
            &ctx,
            part1,
            len1
        );
    }

    if (part2 != nullptr && len2 > 0) {
        mbedtls_sha256_update(
            &ctx,
            part2,
            len2
        );
    }

    if (part3 != nullptr && len3 > 0) {
        mbedtls_sha256_update(
            &ctx,
            part3,
            len3
        );
    }

    mbedtls_sha256_finish(
        &ctx,
        output
    );

    mbedtls_sha256_free(&ctx);
}


// --------------------------------------------------
// Hex helpers
// --------------------------------------------------

int hexValue(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }

    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }

    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }

    return -1;
}


bool hexToBytes(
    const String& hex,
    uint8_t* output,
    size_t outputLength
) {
    if (hex.length() != outputLength * 2) {
        return false;
    }

    for (size_t i = 0; i < outputLength; i++) {
        int high = hexValue(
            hex[i * 2]
        );

        int low = hexValue(
            hex[i * 2 + 1]
        );

        if (high < 0 || low < 0) {
            return false;
        }

        output[i] =
            static_cast<uint8_t>(
                (high << 4) | low
            );
    }

    return true;
}


String bytesToHex(
    const uint8_t* data,
    size_t length
) {
    static const char HEX_CHARS[] =
        "0123456789abcdef";

    String output;

    output.reserve(length * 2);

    for (size_t i = 0; i < length; i++) {
        output +=
            HEX_CHARS[
                (data[i] >> 4) & 0x0F
            ];

        output +=
            HEX_CHARS[
                data[i] & 0x0F
            ];
    }

    return output;
}


// --------------------------------------------------
// Integer encoding helpers
// --------------------------------------------------

void uint32ToLittleEndian(
    uint32_t value,
    uint8_t output[4]
) {
    output[0] =
        static_cast<uint8_t>(
            value & 0xFF
        );

    output[1] =
        static_cast<uint8_t>(
            (value >> 8) & 0xFF
        );

    output[2] =
        static_cast<uint8_t>(
            (value >> 16) & 0xFF
        );

    output[3] =
        static_cast<uint8_t>(
            (value >> 24) & 0xFF
        );
}


void uint64ToLittleEndian(
    uint64_t value,
    uint8_t output[8]
) {
    for (int i = 0; i < 8; i++) {
        output[i] =
            static_cast<uint8_t>(
                (value >> (8 * i))
                & 0xFF
            );
    }
}


uint32_t readUint32LittleEndian(
    const uint8_t* data
) {
    return
        static_cast<uint32_t>(
            data[0]
        )
        |
        (
            static_cast<uint32_t>(
                data[1]
            ) << 8
        )
        |
        (
            static_cast<uint32_t>(
                data[2]
            ) << 16
        )
        |
        (
            static_cast<uint32_t>(
                data[3]
            ) << 24
        );
}


// --------------------------------------------------
// MiniPOW1
// --------------------------------------------------

bool miniPOW1(
    const String& challengeHex,
    uint64_t nonce,
    uint8_t result[32]
) {
    uint8_t challenge[16];

    if (!hexToBytes(
        challengeHex,
        challenge,
        sizeof(challenge)
    )) {
        return false;
    }

    uint8_t nonceBytes[8];

    uint64ToLittleEndian(
        nonce,
        nonceBytes
    );

    uint8_t seed[32];

    sha256Parts(
        challenge,
        sizeof(challenge),
        nonceBytes,
        sizeof(nonceBytes),
        nullptr,
        0,
        seed
    );

    uint8_t previous[32];

    memcpy(
        previous,
        seed,
        32
    );

    for (
        uint32_t blockIndex = 0;
        blockIndex < BLOCK_COUNT;
        blockIndex++
    ) {
        uint8_t indexBytes[4];

        uint32ToLittleEndian(
            blockIndex,
            indexBytes
        );

        uint8_t blockHash[32];

        sha256Parts(
            previous,
            32,
            seed,
            32,
            indexBytes,
            4,
            blockHash
        );

        size_t offset =
            blockIndex * BLOCK_SIZE;

        memcpy(
            scratchpad + offset,
            blockHash,
            32
        );

        memcpy(
            previous,
            blockHash,
            32
        );
    }

    uint8_t state[32];

    memcpy(
        state,
        seed,
        32
    );

    for (
        uint32_t roundIndex = 0;
        roundIndex < MIX_ROUNDS;
        roundIndex++
    ) {
        uint32_t blockIndex =
            readUint32LittleEndian(
                state
            )
            % BLOCK_COUNT;

        size_t offset =
            blockIndex * BLOCK_SIZE;

        uint8_t roundBytes[4];

        uint32ToLittleEndian(
            roundIndex,
            roundBytes
        );

        uint8_t newState[32];

        sha256Parts(
            state,
            32,
            scratchpad + offset,
            32,
            roundBytes,
            4,
            newState
        );

        memcpy(
            state,
            newState,
            32
        );

        for (
            size_t byteIndex = 0;
            byteIndex < BLOCK_SIZE;
            byteIndex++
        ) {
            scratchpad[
                offset + byteIndex
            ] ^= state[byteIndex];
        }
    }

    uint32_t tailIndex =
        readUint32LittleEndian(
            state + 4
        )
        % BLOCK_COUNT;

    size_t tailOffset =
        tailIndex * BLOCK_SIZE;

    sha256Parts(
        state,
        32,
        scratchpad + tailOffset,
        32,
        seed,
        32,
        result
    );

    return true;
}


// --------------------------------------------------
// Numeric target comparison
//
// SHA-256 digest and target are interpreted as
// unsigned 256-bit BIG-ENDIAN integers.
//
// Valid when:
//
// digest <= target
// --------------------------------------------------

bool hashMeetsTarget(
    const uint8_t hash[32],
    const uint8_t target[32]
) {
    for (size_t i = 0; i < 32; i++) {
        if (hash[i] < target[i]) {
            return true;
        }

        if (hash[i] > target[i]) {
            return false;
        }
    }

    return true;
}


// --------------------------------------------------
// MiniPOW self-test
// --------------------------------------------------

bool runSelfTest() {
    Serial.println();
    Serial.println(
        "Running MiniPOW1 self-test..."
    );

    uint8_t result[32];

    unsigned long started =
        millis();

    bool ok = miniPOW1(
        TEST_CHALLENGE,
        TEST_NONCE,
        result
    );

    unsigned long elapsed =
        millis() - started;

    if (!ok) {
        Serial.println(
            "MiniPOW1 self-test ERROR"
        );

        return false;
    }

    String resultHex =
        bytesToHex(
            result,
            32
        );

    Serial.print(
        "Result:   "
    );

    Serial.println(
        resultHex
    );

    Serial.print(
        "Expected: "
    );

    Serial.println(
        TEST_RESULT
    );

    Serial.print(
        "Time:     "
    );

    Serial.print(
        elapsed
    );

    Serial.println(
        " ms"
    );

    if (
        resultHex
        != String(TEST_RESULT)
    ) {
        Serial.println(
            "MiniPOW1 self-test FAILED"
        );

        return false;
    }

    Serial.println(
        "MiniPOW1 self-test PASSED"
    );

    Serial.println();

    return true;
}


// --------------------------------------------------
// Wi-Fi reconnect
// --------------------------------------------------

bool reconnectWiFi() { return connectSavedWiFi(); }

// --------------------------------------------------
// TCP helpers
// --------------------------------------------------

void sendLine(
    const String& message
) {
    client.print(message);
    client.print("\n");
}


String readLine(
    unsigned long timeoutMs = 15000
) {
    String line;

    unsigned long started =
        millis();

    while (
        millis() - started
        < timeoutMs
    ) {
        while (client.available()) {
            char c = client.read();

            if (c == '\n') {
                line.trim();

                return line;
            }

            if (c != '\r') {
                line += c;
            }
        }

        if (!client.connected()) {
            return "";
        }
        delay(1);
    }

    return "";
}


// --------------------------------------------------
// GitHub pool discovery
//
// Official network configuration:
//   https://raw.githubusercontent.com/MiniMinersProject/network/main/pools.json
//
// Expected JSON fields:
//   "version": 1
//   "protocol": "MINIMINERS/0.3"
//   "pools": [ { "host": "...", "port": 2811, "enabled": true }, ... ]
//
// The parser intentionally only extracts the small set of fields the miner
// needs, avoiding another Arduino library dependency. Pools are tried in the
// order they appear in the file. The last valid list is cached in NVS. If
// GitHub is unavailable, the cached list is used. A brand-new miner with no
// cache uses the embedded factory pool.
// --------------------------------------------------
void useFactoryPoolList() {
    poolCount = 1;
    poolHosts[0] = FACTORY_POOL_HOST;
    poolPorts[0] = FACTORY_POOL_PORT;
    currentPoolIndex = 0;
    poolHost = poolHosts[0];
    poolPort = poolPorts[0];
}

bool extractJsonString(const String& body, const String& key, int from, String& value, int& valueEnd) {
    String needle = "\"" + key + "\"";
    int k = body.indexOf(needle, from);
    if (k < 0) return false;
    int colon = body.indexOf(':', k + needle.length());
    if (colon < 0) return false;
    int q1 = body.indexOf('"', colon + 1);
    if (q1 < 0) return false;
    int q2 = body.indexOf('"', q1 + 1);
    if (q2 < 0) return false;
    value = body.substring(q1 + 1, q2);
    valueEnd = q2 + 1;
    return true;
}

bool extractJsonLong(const String& body, const String& key, int from, long& value, int& valueEnd) {
    String needle = "\"" + key + "\"";
    int k = body.indexOf(needle, from);
    if (k < 0) return false;
    int colon = body.indexOf(':', k + needle.length());
    if (colon < 0) return false;
    int p = colon + 1;
    while (p < (int)body.length() && isspace((unsigned char)body[p])) p++;
    int e = p;
    if (e < (int)body.length() && body[e] == '-') e++;
    while (e < (int)body.length() && isdigit((unsigned char)body[e])) e++;
    if (e == p) return false;
    value = body.substring(p, e).toInt();
    valueEnd = e;
    return true;
}

bool parsePoolList(const String& body, bool saveToNvs) {
    String protocol;
    int dummyEnd = 0;
    if (!extractJsonString(body, "protocol", 0, protocol, dummyEnd)) return false;
    if (protocol != PROTOCOL_VERSION) return false;

    long version = 0;
    if (!extractJsonLong(body, "version", 0, version, dummyEnd) || version < 1) return false;

    String hosts[MAX_POOLS];
    uint16_t ports[MAX_POOLS];
    uint8_t count = 0;

    int poolsKey = body.indexOf("\"pools\"");
    if (poolsKey < 0) return false;
    int arrayStart = body.indexOf('[', poolsKey);
    if (arrayStart < 0) return false;
    int arrayEnd = body.indexOf(']', arrayStart);
    if (arrayEnd < 0) return false;

    int pos = arrayStart + 1;
    while (pos < arrayEnd && count < MAX_POOLS) {
        int objStart = body.indexOf('{', pos);
        if (objStart < 0 || objStart >= arrayEnd) break;
        int objEnd = body.indexOf('}', objStart);
        if (objEnd < 0 || objEnd > arrayEnd) break;
        String obj = body.substring(objStart, objEnd + 1);

        // Skip explicitly disabled entries. If "enabled" is omitted, treat it
        // as enabled so future minimal pool entries remain compatible.
        int enabledKey = obj.indexOf("\"enabled\"");
        bool enabled = true;
        if (enabledKey >= 0) {
            int colon = obj.indexOf(':', enabledKey);
            if (colon >= 0) {
                String tail = obj.substring(colon + 1);
                tail.trim();
                if (tail.startsWith("false")) enabled = false;
            }
        }

        if (enabled) {
            String host;
            long port = 0;
            int end1 = 0, end2 = 0;
            if (extractJsonString(obj, "host", 0, host, end1) &&
                extractJsonLong(obj, "port", 0, port, end2)) {
                host.trim();
                if (host.length() > 0 && port >= 1 && port <= 65535) {
                    hosts[count] = host;
                    ports[count] = (uint16_t)port;
                    count++;
                }
            }
        }
        pos = objEnd + 1;
    }

    if (count == 0) return false;

    poolCount = count;
    for (uint8_t i = 0; i < poolCount; i++) {
        poolHosts[i] = hosts[i];
        poolPorts[i] = ports[i];
    }
    currentPoolIndex = 0;
    poolHost = poolHosts[0];
    poolPort = poolPorts[0];

    if (saveToNvs) {
        preferences.begin("miniminer", false);
        preferences.putString("poollist", body);
        preferences.end();
    }
    return true;
}

void loadCachedPool() {
    preferences.begin("miniminer", true);
    String cached = preferences.getString("poollist", "");
    preferences.end();
    if (cached.length() > 0 && parsePoolList(cached, false)) {
        Serial.print("Loaded cached pool list: ");
        Serial.print(poolCount); Serial.println(" pool(s).");
        return;
    }
    useFactoryPoolList();
    Serial.println("No valid cached pool list; using factory fallback.");
}

bool discoverPoolsFromGitHub(bool forceCheck = false) {
    if (WiFi.status() != WL_CONNECTED) return false;
    unsigned long now = millis();
    if (!forceCheck && lastPoolDiscoveryCheck != 0 &&
        now - lastPoolDiscoveryCheck < POOL_DISCOVERY_REFRESH_MS) return false;
    lastPoolDiscoveryCheck = now;

    WiFiClientSecure secureClient;
    // TLS encrypts transport. The network file should later be signed so pool
    // authorization does not depend solely on the hosting account/TLS path.
    secureClient.setInsecure();
    HTTPClient http;
    http.setTimeout(POOL_DISCOVERY_TIMEOUT_MS);
    if (!http.begin(secureClient, POOL_DISCOVERY_URL)) {
        Serial.println("GitHub pool discovery could not start; using cached list.");
        return false;
    }
    int code = http.GET();
    if (code != HTTP_CODE_OK) {
        Serial.print("GitHub pool discovery unavailable (HTTP ");
        Serial.print(code); Serial.println("); using cached list.");
        http.end();
        return false;
    }
    String body = http.getString();
    http.end();
    if (!parsePoolList(body, true)) {
        Serial.println("GitHub pool list invalid; keeping cached list.");
        return false;
    }
    Serial.print("GitHub pool list accepted: ");
    Serial.print(poolCount); Serial.println(" pool(s).");
    return true;
}

bool selectNextPool() {
    if (poolCount <= 1) return false;
    currentPoolIndex = (currentPoolIndex + 1) % poolCount;
    poolHost = poolHosts[currentPoolIndex];
    poolPort = poolPorts[currentPoolIndex];
    return true;
}

// --------------------------------------------------
// Pool connection
// --------------------------------------------------

bool connectPool() {
    if (client.connected()) client.stop();

    currentStatus = "POOL";
    Serial.print("Connecting to Mini Miners pool ");
    Serial.print(poolHost);
    Serial.print(":");
    Serial.println(poolPort);

    if (!client.connect(poolHost.c_str(), poolPort)) {
        Serial.println("Pool connection failed.");
        currentStatus = "POOL FAIL";
        return false;
    }

    String hello = readLine();
    Serial.print("Server: ");
    Serial.println(hello);

    String expectedHello = String("HELLO|") + PROTOCOL_VERSION;
    if (hello != expectedHello) {
        Serial.println("Unexpected protocol version.");
        currentStatus = "PROTO ERR";
        client.stop();
        return false;
    }

    if (!identityReady) {
        Serial.println("Cryptographic identity is not ready.");
        currentStatus = "ID FAIL";
        client.stop();
        return false;
    }

    // Identity-aware AUTH. Older miners still use the original four-field AUTH;
    // the server remains backward-compatible with those devices.
    String auth = String("AUTH_WALLET|")
        + walletAddress + "|"
        + minerName + "|"
        + MINER_TYPE + "|"
        + identityPublicKeyB64;

    sendLine(auth);

    String challengeResponse = readLine();
    Serial.print("Identity: ");
    Serial.println(challengeResponse);

    if (!challengeResponse.startsWith("CHALLENGE|")) {
        if (challengeResponse.indexOf("WALLET") >= 0 || challengeResponse.indexOf("REGISTER") >= 0 || challengeResponse.indexOf("AUTH_FAIL") >= 0) {
            Serial.println("MINING AUTHENTICATION FAILED.");
            Serial.println("This wallet is not registered/authorized. Go to miniminers.org/setup and create/register an account first, then enter that wallet in this miner.");
        }
        Serial.println("Server did not issue an identity challenge.");
        currentStatus = "ID CHAL ERR";
        client.stop();
        return false;
    }

    String challenge = challengeResponse.substring(10);
    challenge.trim();

    String signatureB64 = signIdentityChallenge(challenge);
    if (signatureB64.length() == 0) {
        currentStatus = "SIGN FAIL";
        client.stop();
        return false;
    }

    sendLine(String("AUTH_RESPONSE|") + signatureB64);

    String authResponse = readLine();
    Serial.print("Auth:   ");
    Serial.println(authResponse);

    if (!authResponse.startsWith("AUTH_OK|")) {
        currentStatus = "AUTH FAIL";
        client.stop();
        return false;
    }

    String serverFingerprint = authResponse.substring(8);
    serverFingerprint.trim();

    if (!serverFingerprint.equalsIgnoreCase(identityFingerprint)) {
        Serial.println("Identity fingerprint mismatch.");
        Serial.print("Local:  ");
        Serial.println(identityFingerprint);
        Serial.print("Server: ");
        Serial.println(serverFingerprint);
        currentStatus = "ID MISMATCH";
        client.stop();
        return false;
    }

    Serial.print("Identity verified: ");
    Serial.println(identityFingerprint);

    armMiningWatchdog();

    Serial.println();
    Serial.println("MiniPOW1 mining started.");
    Serial.println();

    currentStatus = "MINING";
    return true;
}


// --------------------------------------------------
// Mine one server job
// --------------------------------------------------

bool mineOneJob() {
    currentStatus = "GET JOB";
    sendLine(
        "JOB"
    );

    String job =
        readLine();

    if (
        job.length() == 0
        || !job.startsWith("JOB|")
    ) {
        Serial.print(
            "Bad job response: "
        );

        Serial.println(job);

        return false;
    }

    int separator1 =
        job.indexOf('|');

    int separator2 =
        job.indexOf(
            '|',
            separator1 + 1
        );

    int separator3 =
        job.indexOf(
            '|',
            separator2 + 1
        );

    if (
        separator1 < 0
        || separator2 < 0
        || separator3 < 0
    ) {
        Serial.println(
            "Malformed JOB response."
        );

        return false;
    }

    String jobId =
        job.substring(
            separator1 + 1,
            separator2
        );

    String challenge =
        job.substring(
            separator2 + 1,
            separator3
        );

    String targetHex =
        job.substring(
            separator3 + 1
        );

    targetHex.trim();
    noteMiningProgress();

    if (
        targetHex.length() != 64
    ) {
        Serial.println(
            "Invalid target length."
        );

        return false;
    }

    uint8_t target[32];

    if (
        !hexToBytes(
            targetHex,
            target,
            32
        )
    ) {
        Serial.println(
            "Invalid target."
        );

        return false;
    }

    Serial.print(
        "Mining job "
    );

    Serial.println(
        jobId
    );

    Serial.print(
        "Target: "
    );

    Serial.print(
        targetHex.substring(
            0,
            16
        )
    );

    Serial.println("...");

    uint64_t nonce = 0;
    uint32_t attempts = 0;

    unsigned long started =
        millis();

    unsigned long speedTimer = started;
    uint32_t speedAttempts = 0;

    currentStatus = "MINING";
    uint8_t digest[32];

    while (true) {
        if (
            WiFi.status()
            != WL_CONNECTED
        ) {
            Serial.println(
                "Wi-Fi disconnected."
            );

            return false;
        }

        if (!client.connected()) {
            Serial.println(
                "Pool disconnected."
            );

            return false;
        }

        if (
            !miniPOW1(
                challenge,
                nonce,
                digest
            )
        ) {
            Serial.println(
                "MiniPOW calculation error."
            );

            return false;
        }

        attempts++;
        speedAttempts++;

        unsigned long now = millis();
        if (now - speedTimer >= 1000) {
            float seconds = (now - speedTimer) / 1000.0f;
            if (seconds > 0.0f) {
                currentSpeed = speedAttempts / seconds;
            }
            speedTimer = now;
            speedAttempts = 0;
        }

        // Keep LED timing and BOOT responsive while the CPU is hashing.
        // This also makes the accepted-share pulse reliably human-visible.
        serviceStatusLed();
        serviceSetupButton();
        if (setupMode) return false;

        if (
            hashMeetsTarget(
                digest,
                target
            )
        ) {
            break;
        }

        nonce++;
    }

    unsigned long elapsedMs =
        millis() - started;

    float elapsedSeconds =
        elapsedMs / 1000.0f;

    float speed = 0.0f;

    if (elapsedSeconds > 0) {
        speed =
            attempts
            / elapsedSeconds;
    }

    currentSpeed = speed;
    lastShareSeconds = elapsedSeconds;
    lastAttempts = attempts;
    completedJobs++;

    Serial.print(
        "Found nonce "
    );

    Serial.print(
        static_cast<unsigned long>(
            nonce
        )
    );

    Serial.print(
        " | "
    );

    Serial.print(
        attempts
    );

    Serial.print(
        " attempts | "
    );

    Serial.print(
        speed,
        2
    );

    Serial.print(
        " MiniPOW/s | "
    );

    Serial.print(
        elapsedSeconds,
        2
    );

    Serial.println(
        "s"
    );

    currentStatus = "SUBMIT";
       String submit =
            String("SUBMIT|")
            + jobId
            + "|"
            + String(
                static_cast<unsigned long>(
                    nonce
               )
            )
            + "|"
            + String(currentSpeed, 2);

    sendLine(
        submit
    );

    String result =
        readLine();

    Serial.println(
        result
    );

    Serial.println();

    if (
        result.length() == 0
    ) {
        return false;
    }

    String upperResult = result;
    upperResult.toUpperCase();

    if (upperResult.indexOf("ACCEPT") >= 0 || upperResult == "OK") {
        acceptedShares++;
        currentStatus = "ACCEPTED";
        flashAcceptedShare();
        serviceStatusLed();

        // Server response format:
        // ACCEPTED|reward|digest|expected_attempts
        int p1 = result.indexOf('|');
        int p2 = result.indexOf('|', p1 + 1);
        int p3 = result.indexOf('|', p2 + 1);

        if (p1 >= 0 && p2 > p1) {
            float reward = result.substring(p1 + 1, p2).toFloat();
            if (reward >= 0.0f) {
                sessionEarnedMini += reward;
            }
        }

        if (p3 > p2) {
            uint32_t diff = (uint32_t) result.substring(p3 + 1).toInt();
            if (diff > 0) {
                currentDifficulty = diff;
            }
        }
    } else if (upperResult.indexOf("REJECT") >= 0 || upperResult.indexOf("INVALID") >= 0) {
        rejectedShares++;
        currentStatus = "REJECTED";
    } else {
        currentStatus = "SUBMITTED";
    }
    noteMiningProgress();
    return true;
}



// Keep the BOOT/setup button responsive during normal mining.
void serviceSetupButton() {
    if (setupMode) return;

    if (digitalRead(SETUP_BOOT_PIN) == LOW) {
        delay(25);  // debounce
        if (digitalRead(SETUP_BOOT_PIN) == LOW) {
            Serial.println("BOOT pressed: entering setup mode.");
            startSetupMode();
        }
    }
}

// Replacement for long blocking retry delays.
// This keeps both the setup button and status LED responsive while waiting.
bool waitWithSetupButton(unsigned long waitMs) {
    unsigned long started = millis();

    while (millis() - started < waitMs) {
        serviceSetupButton();
        serviceStatusLed();

        if (setupMode) return false;

        delay(20);
    }

    return true;
}

void setup() {
    pinMode(STATUS_LED_PIN, OUTPUT);
    writeStatusLed(false);
    pinMode(SETUP_BOOT_PIN, INPUT_PULLUP);
    delay(50);
    bool bootSetupRequested = (digitalRead(SETUP_BOOT_PIN) == LOW);
    Serial.begin(115200);

    delay(1000);
    Serial.println();
    Serial.println(
        "Mini Miners ESP32"
    );

    Serial.println(
        "MiniPOW1 Network Miner"
    );
    Serial.print("Firmware: ");
    Serial.println(FIRMWARE_VERSION);

    Serial.println();

    if (!runSelfTest()) {
        Serial.println(
            "Mining disabled because "
            "self-test failed."
        );

        while (true) {
            delay(1000);
        }
    }

    loadSavedConfiguration();

    if (bootSetupRequested) {
        Serial.println("BOOT detected at startup: entering manual setup mode.");
        startSetupMode();
        return;
    }

    if (!loadOrCreateIdentity()) {
        Serial.println("Cryptographic identity initialization failed.");
        while (true) delay(1000);
    }

    Serial.print("Wallet: ");
    Serial.println(walletAddress.length() > 0 ? walletAddress : "(not configured)");
    Serial.print("Miner name: ");
    Serial.println(minerName);

    if (savedSSID.length() == 0 || walletAddress.length() == 0 || minerName.length() == 0) {
        Serial.println("Setup required: Wi-Fi, registered wallet, and miner name must be configured.");
        startSetupMode();
        return;
    }

    if (!connectSavedWiFi()) {
        if (setupMode) return;
        Serial.println("Saved Wi-Fi unavailable. Normal mode will keep retrying; press BOOT once to reconfigure.");
    }

    loadCachedPool();
    discoverPoolsFromGitHub(true);


    currentStatus = "READY";
}


// --------------------------------------------------
// Main loop
// --------------------------------------------------

void loop() {
    if (setupMode) {
        runSetupMode();
        return;
    }

    serviceSetupButton();
    serviceStatusLed();
    if (setupMode) return;
    serviceMiningWatchdog();

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Wi-Fi lost. Reconnecting...");
        currentStatus = "WIFI LOST";
        statusLedMode = LED_CONNECTION;
        reconnectCount++;
        if (!reconnectWiFi()) {
            Serial.println("Wi-Fi reconnect failed. Will retry. Press BOOT once to change settings.");
            if (!waitWithSetupButton(5000)) return;
            return;
        }
    }

    if (!client.connected()) {
        if (!connectPool()) {
            selectNextPool();
            currentStatus = "POOL RETRY";
            statusLedMode = LED_CONNECTION;
            Serial.println(
                "Retrying pool in 5 seconds..."
            );

            if (!waitWithSetupButton(5000)) return;

            return;
        }
    }

    if (!mineOneJob()) {
        client.stop();
        currentStatus = "RECONNECT";
        Serial.println(
            "Mining session interrupted."
        );

        Serial.println(
            "Reconnecting in 5 seconds..."
        );

        if (!waitWithSetupButton(5000)) return;
    } else {
        // Mining stays the priority. Pool discovery maintenance happens between jobs.
        discoverPoolsFromGitHub(false);
    }
}

