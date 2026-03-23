#ifndef SIMPLE_OTA_H
#define SIMPLE_OTA_H

#include <Arduino.h>
#include <Preferences.h>
#include <Update.h>
#include <WebServer.h>
#include <functional>

// --- Callbacks & Constants ---
typedef std::function<void()> OTACallback;
typedef std::function<void(int, int)> OTAProgressCallback;
typedef std::function<void(int)> OTAErrorCallback;

#define OTA_ERROR_NO_FILE 100
#define OTA_ERROR_FILE_READ 101
#define OTA_ERROR_MANIFEST 102
#define OTA_ERROR_STREAM 103

#define DEFAULT_INTERVAL 3600000 // 1 hr (ms)

class SimpleOTA {
public:
  SimpleOTA();
  ~SimpleOTA();

  // --- Setup & Lifecycle ---
  void begin(const char *version, WebServer *server = nullptr);
  void useServer(WebServer *server);

  // --- Configuration ---
  void setVersion(const char *version);
  String getVersion();
  void setAuth(const char *username, const char *password);
  const char *getAuthUser();
  const char *getAuthPass();
  void setManifestUrl(const char *url);
  String getManifestUrl();
  void setAutoCheckInterval(unsigned long minutes);
  unsigned long getAutoCheckInterval();

  void setDeviceID(const char *id);
  const char *getDeviceID() const;

  void saveConfigs(const char *url, unsigned long hours, const char *deviceID);

  void clearSettings();
  size_t getOtaSize();
  bool isRebootRequired();
  String getLastMessage();

  // --- Update Operations ---
  void onlineUpdate(const char *manifestUrl = nullptr);
  void fileUpdate(const char *path, const char *version = "");
  bool rollback();

  // --- Callbacks ---
  void onUpdateStart(OTACallback cb);
  void onUpdateEnd(OTACallback cb);
  void onUpdateProgress(OTAProgressCallback cb);
  void onUpdateError(OTAErrorCallback cb);

private:
  // --- Properties ---
  char _currentVersion[32];
  char _authUser[32];
  char _authPass[32];
  size_t _otaSize = 0;
  bool _rebootRequired = false;
  volatile bool _forceCheck = false;
  bool _isUpdating = false;
  int _otaProgress = 0;
  char _deviceID[32];
  String _lastMessage = "";

  // --- Web Server & Routing ---
  WebServer *_server;
  bool _internalServer;
  void _setupRoutes();
  void _handleOTAIndex();
  void _handleOTAUpdate();
  void _handleOTAStatus();
  void _handleOTARollback();

  // --- Background Task & Storage ---
  Preferences _prefs;
  String _manifestUrl = "";
  unsigned long _autoCheckInterval = DEFAULT_INTERVAL; // ms
  unsigned long _lastCheckTime = 0;
  TaskHandle_t _taskHandle = NULL;
  static void _loopTask(void *param);

  // --- Callbacks ---
  OTACallback _startCallback;
  OTACallback _endCallback;
  OTAProgressCallback _progressCallback;
  OTAErrorCallback _errorCallback;

  // --- Helpers ---
  bool _parseManifest(const String &content, String &url, String &version,
                      String &md5);
  String _getContentType(String filename);
};

extern SimpleOTA OTA;

#endif
