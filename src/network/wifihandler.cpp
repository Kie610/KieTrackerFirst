/*
	SlimeVR Code is placed under the MIT license
	Copyright (c) 2021 Eiren Rain & SlimeVR contributors

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in
	all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
	THE SOFTWARE.
*/
#include "network/wifihandler.h"

#include "GlobalVars.h"
#include "globals.h"
#if !ESP8266
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#endif

namespace SlimeVR {

#if !ESP8266
namespace {

// The event callback has to be static, so it cannot reach the instance logger.
SlimeVR::Logging::Logger wifiEventLogger{"WiFiHandler"};

// WiFi.status() only exposes WL_* states, so a disconnect that the radio
// understood precisely arrives here as a vague "Timeout". The IDF reason code is
// what actually says whether the AP went away, the handshake failed, or the
// station gave up on beacons, and the last one is invisible without it. Router
// logs see only what the station transmitted -- reason 8 "leaving BSS" looks
// like a deliberate disconnect from the AP side even when the real cause was
// that beacons stopped arriving at the edge of range.
const char* wifiDisconnectReasonName(uint8_t reason) {
	switch (reason) {
		case WIFI_REASON_AUTH_EXPIRE: return "AUTH_EXPIRE";
		case WIFI_REASON_AUTH_LEAVE: return "AUTH_LEAVE";
		case WIFI_REASON_ASSOC_EXPIRE: return "ASSOC_EXPIRE";
		case WIFI_REASON_ASSOC_TOOMANY: return "ASSOC_TOOMANY (AP is full)";
		case WIFI_REASON_NOT_AUTHED: return "NOT_AUTHED";
		case WIFI_REASON_NOT_ASSOCED: return "NOT_ASSOCED";
		case WIFI_REASON_ASSOC_LEAVE: return "ASSOC_LEAVE (we left)";
		case WIFI_REASON_HANDSHAKE_TIMEOUT: return "HANDSHAKE_TIMEOUT (wrong password?)";
		case WIFI_REASON_NO_AP_FOUND: return "NO_AP_FOUND (out of range or wrong SSID)";
		case WIFI_REASON_AUTH_FAIL: return "AUTH_FAIL";
		case WIFI_REASON_ASSOC_FAIL: return "ASSOC_FAIL";
		case WIFI_REASON_BEACON_TIMEOUT: return "BEACON_TIMEOUT (signal too weak)";
		case WIFI_REASON_CONNECTION_FAIL: return "CONNECTION_FAIL";
		default: return "see esp_wifi_types.h";
	}
}

}  // namespace

void WiFiNetwork::onWiFiEvent(arduino_event_id_t event, arduino_event_info_t info) {
	switch (event) {
		case ARDUINO_EVENT_WIFI_STA_CONNECTED: {
			const auto& e = info.wifi_sta_connected;
			wifiEventLogger.info(
				"Associated on channel %u, BSSID %02x:%02x:%02x:%02x:%02x:%02x, RSSI %d "
				"dBm",
				e.channel,
				e.bssid[0],
				e.bssid[1],
				e.bssid[2],
				e.bssid[3],
				e.bssid[4],
				e.bssid[5],
				WiFi.RSSI()
			);
			break;
		}
		case ARDUINO_EVENT_WIFI_STA_DISCONNECTED: {
			const auto reason = info.wifi_sta_disconnected.reason;
			// RSSI is only meaningful while associated; once the link is down the
			// driver reports 0, which reads like a measurement and is not one.
			const int rssi = WiFi.RSSI();
			if (rssi != 0) {
				wifiEventLogger.warn(
					"Disconnected: reason %u (%s), RSSI %d dBm",
					reason,
					wifiDisconnectReasonName(reason),
					rssi
				);
			} else {
				wifiEventLogger.warn(
					"Disconnected: reason %u (%s), RSSI unavailable (not associated)",
					reason,
					wifiDisconnectReasonName(reason)
				);
			}
			break;
		}
		default: break;
	}
}
#endif

void WiFiNetwork::reportWifiProgress() {
	if (lastWifiReportTime + 1000 < millis()) {
		lastWifiReportTime = millis();
		Serial.print(".");
	}
}

void WiFiNetwork::setStaticIPIfDefined() {
#ifdef WIFI_USE_STATICIP
	const IPAddress ip(WIFI_STATIC_IP);
	const IPAddress gateway(WIFI_STATIC_GATEWAY);
	const IPAddress subnet(WIFI_STATIC_SUBNET);
	WiFi.config(ip, gateway, subnet);
#endif
}

bool WiFiNetwork::isConnected() const {
	return wifiState == WiFiReconnectionStatus::Success;
}

void WiFiNetwork::setWiFiCredentials(const char* SSID, const char* pass) {
	wifiProvisioning.stopProvisioning();
	tryConnecting(false, SSID, pass);
	retriedOnG = false;
	// Reset state, will get back into provisioning if can't connect
	hadWifi = false;
	wifiState = WiFiReconnectionStatus::ServerCredAttempt;
}

IPAddress WiFiNetwork::getAddress() { return WiFi.localIP(); }

void WiFiNetwork::setUp() {
	wifiHandlerLogger.info("Setting up WiFi");
	WiFi.persistent(true);
	WiFi.mode(WIFI_STA);
	WiFi.hostname("SlimeVR FBT Tracker");
#if !ESP8266
	WiFi.onEvent(onWiFiEvent);
#endif
	wifiHandlerLogger.info(
		"Loaded credentials for SSID '%s' and pass length %d",
		getSSID().c_str(),
		getPassword().length()
	);

	trySavedCredentials();

#if ESP8266
#if POWERSAVING_MODE == POWER_SAVING_NONE
	WiFi.setSleepMode(WIFI_NONE_SLEEP);
#elif POWERSAVING_MODE == POWER_SAVING_MINIMUM
	WiFi.setSleepMode(WIFI_MODEM_SLEEP);
#elif POWERSAVING_MODE == POWER_SAVING_MODERATE
	WiFi.setSleepMode(WIFI_MODEM_SLEEP, 10);
#elif POWERSAVING_MODE == POWER_SAVING_MAXIMUM
	WiFi.setSleepMode(WIFI_LIGHT_SLEEP, 10);
#error "MAX POWER SAVING NOT WORKING YET, please disable!"
#endif
#else
#if POWERSAVING_MODE == POWER_SAVING_NONE
	WiFi.setSleep(WIFI_PS_NONE);
#elif POWERSAVING_MODE == POWER_SAVING_MINIMUM
	WiFi.setSleep(WIFI_PS_MIN_MODEM);
#elif POWERSAVING_MODE == POWER_SAVING_MODERATE \
	|| POWERSAVING_MODE == POWER_SAVING_MAXIMUM
	wifi_config_t conf;
	if (esp_wifi_get_config(WIFI_IF_STA, &conf) == ESP_OK) {
		conf.sta.listen_interval = 10;
		esp_wifi_set_config(WIFI_IF_STA, &conf);
		WiFi.setSleep(WIFI_PS_MAX_MODEM);
	} else {
		wifiHandlerLogger.error("Unable to get WiFi config, power saving not enabled!");
	}
#endif
#endif
}

void WiFiNetwork::onConnected() {
	wifiState = WiFiReconnectionStatus::Success;
	wifiProvisioning.stopProvisioning();
	statusManager.setStatus(SlimeVR::Status::WIFI_CONNECTING, false);
	hadWifi = true;
	wifiHandlerLogger.info(
		"Connected successfully to SSID '%s', IP address %s",
		getSSID().c_str(),
		WiFi.localIP().toString().c_str()
	);
	// Reset it, in case we just connected with server creds
}

String WiFiNetwork::getSSID() {
#if ESP8266
	return WiFi.SSID();
#else
	// Necessary, because without a WiFi.begin(), ESP32 is not kind enough to load the
	// SSID on its own, for whatever reason
	wifi_config_t wifiConfig;
	esp_wifi_get_config((wifi_interface_t)ESP_IF_WIFI_STA, &wifiConfig);
	return {reinterpret_cast<char*>(wifiConfig.sta.ssid)};
#endif
}

String WiFiNetwork::getPassword() {
#if ESP8266
	return WiFi.psk();
#else
	// Same as above
	wifi_config_t wifiConfig;
	esp_wifi_get_config((wifi_interface_t)ESP_IF_WIFI_STA, &wifiConfig);
	return {reinterpret_cast<char*>(wifiConfig.sta.password)};
#endif
}

WiFiNetwork::WiFiReconnectionStatus WiFiNetwork::getWiFiState() { return wifiState; }

void WiFiNetwork::upkeep() {
	wifiProvisioning.upkeepProvisioning();

	if (WiFi.status() == WL_CONNECTED) {
		if (!isConnected()) {
			onConnected();
			return;
		}

		if (millis() - lastRssiSample >= 2000) {
			lastRssiSample = millis();
			uint8_t signalStrength = WiFi.RSSI();
			networkConnection.sendSignalStrength(signalStrength);
		}
		return;
	}

	if (isConnected()) {
		statusManager.setStatus(SlimeVR::Status::WIFI_CONNECTING, true);
		wifiHandlerLogger.warn("Connection to WiFi lost, reconnecting...");
		trySavedCredentials();
		return;
	}

	if (wifiState != WiFiReconnectionStatus::Failed) {
		reportWifiProgress();
	}

	if (millis() - wifiConnectionTimeout
			< static_cast<uint32_t>(WiFiTimeoutSeconds * 1000)
		&& WiFi.status() == WL_DISCONNECTED) {
		return;
	}

	switch (wifiState) {
		case WiFiReconnectionStatus::NotSetup:  // Wasn't set up
			return;
		case WiFiReconnectionStatus::SavedAttempt:  // Couldn't connect with
													// first set of
													// credentials
			if (!trySavedCredentials()) {
				tryHardcodedCredentials();
			}
			return;
		case WiFiReconnectionStatus::HardcodeAttempt:  // Couldn't connect with
													   // second set of credentials
			if (!tryHardcodedCredentials()) {
				wifiState = WiFiReconnectionStatus::Failed;
			}
			return;
		case WiFiReconnectionStatus::ServerCredAttempt:  // Couldn't connect with
														 // server-sent credentials.
			if (!tryServerCredentials()) {
				wifiState = WiFiReconnectionStatus::Failed;
			}
			return;
		case WiFiReconnectionStatus::Failed:  // Couldn't connect with second set of
											  // credentials or server credentials
// Return to the default PHY Mode N.
#if ESP8266
			if constexpr (USE_ATTENUATION) {
				WiFi.setOutputPower(20.0 - ATTENUATION_N);
			}
			WiFi.setPhyMode(WIFI_PHY_MODE_11N);
#endif
			// Start smart config
			if (!hadWifi && !WiFi.smartConfigDone()
				&& millis() - wifiConnectionTimeout
					   >= static_cast<uint32_t>(WiFiTimeoutSeconds * 1000)) {
				if (WiFi.status() != WL_IDLE_STATUS) {
					wifiHandlerLogger.error(
						"Can't connect from any credentials, error: %d, reason: %s.",
						static_cast<int>(statusToFailure(WiFi.status())),
						statusToReasonString(WiFi.status())
					);
					wifiConnectionTimeout = millis();
				}
				wifiProvisioning.startProvisioning();
			}
			return;
		default:
			return;
	}
}

const char* WiFiNetwork::statusToReasonString(wl_status_t status) {
	switch (status) {
		case WL_DISCONNECTED:
			return "Timeout";
#ifdef ESP8266
		case WL_WRONG_PASSWORD:
			return "Wrong password";
		case WL_CONNECT_FAILED:
			return "Connection failed";
#elif ESP32
		case WL_CONNECT_FAILED:
			return "Wrong password";
#endif

		case WL_NO_SSID_AVAIL:
			return "SSID not found";
		default:
			return "Unknown";
	}
}

WiFiNetwork::WiFiFailureReason WiFiNetwork::statusToFailure(wl_status_t status) {
	switch (status) {
		case WL_DISCONNECTED:
			return WiFiFailureReason::Timeout;
#ifdef ESP8266
		case WL_WRONG_PASSWORD:
			return WiFiFailureReason::WrongPassword;
#elif ESP32
		case WL_CONNECT_FAILED:
			return WiFiFailureReason::WrongPassword;
#endif

		case WL_NO_SSID_AVAIL:
			return WiFiFailureReason::SSIDNotFound;
		default:
			return WiFiFailureReason::Unknown;
	}
}

void WiFiNetwork::showConnectionAttemptFailed(const char* type) const {
	wifiHandlerLogger.error(
		"Can't connect from %s credentials, error: %d, reason: %s.",
		type,
		static_cast<int>(statusToFailure(WiFi.status())),
		statusToReasonString(WiFi.status())
	);
}

bool WiFiNetwork::trySavedCredentials() {
	if (getSSID().length() == 0) {
		wifiHandlerLogger.debug("Skipping saved credentials attempt on 0-length SSID..."
		);
		wifiState = WiFiReconnectionStatus::HardcodeAttempt;
		return false;
	}

	if (wifiState == WiFiReconnectionStatus::SavedAttempt) {
		showConnectionAttemptFailed("saved");

		if (WiFi.status() != WL_DISCONNECTED) {
			return false;
		}

		if (retriedOnG) {
			return false;
		}

		retriedOnG = true;
		wifiHandlerLogger.debug("Trying saved credentials with PHY Mode G...");
		return tryConnecting(true);
	}

	retriedOnG = false;

	wifiState = WiFiReconnectionStatus::SavedAttempt;
	return tryConnecting();
}

bool WiFiNetwork::tryHardcodedCredentials() {
#if defined(WIFI_CREDS_SSID) && defined(WIFI_CREDS_PASSWD)
	if (wifiState == WiFiReconnectionStatus::HardcodeAttempt) {
		showConnectionAttemptFailed("hardcoded");

		if (WiFi.status() != WL_DISCONNECTED) {
			return false;
		}

		if (retriedOnG) {
			return false;
		}

		retriedOnG = true;
		wifiHandlerLogger.debug("Trying hardcoded credentials with PHY Mode G...");
		// Don't need to save hardcoded credentials
		WiFi.persistent(false);
		auto result = tryConnecting(true, WIFI_CREDS_SSID, WIFI_CREDS_PASSWD);
		WiFi.persistent(true);
		return result;
	}

	retriedOnG = false;

	wifiState = WiFiReconnectionStatus::HardcodeAttempt;
	// Don't need to save hardcoded credentials
	WiFi.persistent(false);
	auto result = tryConnecting(false, WIFI_CREDS_SSID, WIFI_CREDS_PASSWD);
	WiFi.persistent(true);
	return result;
#else
	wifiState = WiFiReconnectionStatus::HardcodeAttempt;
	return false;
#endif
}

bool WiFiNetwork::tryServerCredentials() {
	if (WiFi.status() != WL_DISCONNECTED) {
		return false;
	}

	if (retriedOnG) {
		return false;
	}

	retriedOnG = true;

	return tryConnecting(true);
}

bool WiFiNetwork::tryConnecting(bool phyModeG, const char* SSID, const char* pass) {
#if ESP8266
	if (phyModeG) {
		WiFi.setPhyMode(WIFI_PHY_MODE_11G);
		if constexpr (USE_ATTENUATION) {
			WiFi.setOutputPower(20.0 - ATTENUATION_G);
		}
	} else {
		WiFi.setPhyMode(WIFI_PHY_MODE_11N);
		if constexpr (USE_ATTENUATION) {
			WiFi.setOutputPower(20.0 - ATTENUATION_N);
		}
	}
#else
	if (phyModeG) {
		return false;
	}
#endif

	setStaticIPIfDefined();
	if (SSID == nullptr) {
		WiFi.begin();
	} else {
		WiFi.begin(SSID, pass);
	}
	wifiConnectionTimeout = millis();
	return true;
}

}  // namespace SlimeVR
