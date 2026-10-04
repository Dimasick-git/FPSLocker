#pragma once
#include <curl/curl.h>
#include <stdatomic.h>
#include <zlib.h>
#include "DownloadFiles.hpp"

const unsigned char data[] = {
	#embed "titleids_with_patches.bin"
};

std::array<uint64_t, sizeof(data) / 8>* titleids_needing_patch = (std::array<uint64_t, sizeof(data) / 8>*)&data[0];

struct resolutionCalls {
	uint16_t width;
	uint16_t height;
	uint16_t calls;
};

struct NxFpsSharedBlock {
	uint32_t MAGIC;
	uint8_t FPS;
	float FPSavg;
	bool pluginActive;
	uint8_t FPSlocked;
	uint8_t FPSmode;
	uint8_t ZeroSync;
	uint8_t patchApplied;
	uint8_t API;
	uint32_t FPSticks[10];
	uint8_t Buffers;
	uint8_t SetBuffers;
	uint8_t ActiveBuffers;
	uint8_t SetActiveBuffers;
	union {
		struct {
			bool handheld: 1;
			bool docked: 1;
			unsigned int reserved: 6;
		} PACKED ds;
		uint8_t general;
	} displaySync;
	resolutionCalls renderCalls[8];
	resolutionCalls viewportCalls[8];
	bool forceOriginalRefreshRate;
	bool dontForce60InDocked;
	bool forceSuspend;
	uint8_t currentRefreshRate;
	float readSpeedPerSecond;
	uint8_t FPSlockedDocked;
	uint64_t frameNumber;
	int8_t expectedSetBuffers;
} NX_PACKED;

static_assert(sizeof(NxFpsSharedBlock) == 174);

struct DockedAdditionalSettings {
	bool dontForce60InDocked;
	bool fpsTargetWithoutRRMatchLowest;
	bool displaySyncDockedOutOfFocus60;
};

#include "Langs.hpp"

NxFpsSharedBlock* Shared = 0;
uint8_t* refreshRate_shared = 0;
bool _isDocked = false;
bool _def = true;
bool PluginRunning = false;
bool state = false;
bool closed = false;
bool check = false;
bool SaltySD = false;
bool bak = false;
bool plugin = true;
uint8_t SetBuffers_save = 0;
bool forceSuspend_save = false;
char FPSMode_c[64];
char FPSTarget_c[64];
char PFPS_c[32];
char nvnBuffers[96] = "";
char SyncWait_c[32];
bool displaySyncOutOfFocus60 = false;
#ifdef __SWITCH__
	#define systemtickfrequency 19200000
#elif __OUNCE__
	#define systemtickfrequency 31250000
#else
	uint64_t systemtickfrequency = 19200000;
#endif

char configPath[128] = "";
char patchPath[128] = "";
char savePath[64] = "";
uint64_t PID = 0;
uint64_t TID = 0;
uint64_t BID = 0;

Handle remoteSharedMemory = 1;
SharedMemory _sharedmemory = {};
bool SharedMemoryUsed = false;

Result configValid = 10;
Result patchValid = 0x202;
char lockInvalid[96] = "";
char lockVersionExpected[40] = "";
char patchChar[256] = "";
char patchAppliedChar[64] = "";
uint8_t* patchApplied_shared = 0;
Thread t0;
std::string ZeroSyncMode = "";

bool FileDownloaded = false;
Thread t1;
bool downloadingRunning = false;
Result error_code = UINT32_MAX;
bool curl_timeout = false;
uint8_t supportedHandheldRefreshRates[] = {40, 45, 50, 55, 60};
uint8_t supportedHandheldRefreshRatesOLED[] = {45, 50, 55, 60};
Mutex TitlesAccess;

volatile const bool forceEnglishLanguage = false;
std::string overlayName = "sdmc:/switch/.overlays/";

LEvent threadexit = {0};

// Current libnx includes the compressed HOS 21+ title layout.
using NacpStruct2 = NacpStruct;

struct Title
{
	uint64_t TitleID;
	std::string TitleName;
};

struct DisplayData {
	uint32_t pixelClockkHz;
	uint16_t width;
	uint16_t height;
	float refreshRate;
	uint16_t widthFrontPorch;
	uint16_t heightFrontPorch;
	uint16_t widthSync;
	uint16_t heightSync;
	uint16_t widthBackPorch;
	uint16_t heightBackPorch;
};

std::vector<Title> titles;
std::string TV_name = "Unknown";


bool file_exists(const char *filename)
{
    struct stat buffer;
    return stat(filename, &buffer) == 0 ? true : false;
}

void getDockedHighestRefreshRate(uint8_t* highestRefreshRate, uint8_t* setLinkRate = nullptr, uint8_t* setLaneCount = nullptr) {
	if (SaltySD_Connect()) {
		*highestRefreshRate = 60;
		return;
	}
	uint8_t refreshRate = 60;
	uint8_t linkRate = 10;
	uint8_t laneCount = 0;
	Result rc = SaltySD_GetDockedHighestRefreshRate(&refreshRate, &linkRate, &laneCount);
	SaltySD_Term();
	if (R_SUCCEEDED(rc) && setLinkRate) *setLinkRate = linkRate;
	if (R_SUCCEEDED(rc) && setLaneCount) *setLaneCount = laneCount;
	if (R_SUCCEEDED(rc) && refreshRate > 60) *highestRefreshRate = refreshRate;
	else *highestRefreshRate = 60;

}

void LoadDockedModeAllowedSave(DockedModeRefreshRateAllowed &rr, DockedAdditionalSettings &as, int* displayCRC, bool is720p) {
	for (size_t i = 0; i < sizeof(DockedModeRefreshRateAllowed); i++) {
		if (DockedModeRefreshRateAllowedValues[i] == 60 || DockedModeRefreshRateAllowedValues[i] == 50) rr[i] = true;
		else rr[i] = false;
	}
	as.dontForce60InDocked = false;
	as.fpsTargetWithoutRRMatchLowest = false;
	as.displaySyncDockedOutOfFocus60 = false;
	TV_name = "Unknown";
	tsl::hlp::doWithSmSession([]{
		setsysInitialize();
	});
    SetSysEdid edid = {0};
    if (R_FAILED(setsysGetEdid(&edid))) {
		return;
    }
    char path[128] = "";
	int crc32 = crc32Calculate(&edid, sizeof(edid));
	if (displayCRC) *displayCRC = crc32;
    snprintf(path, sizeof(path), "sdmc:/SaltySD/plugins/FPSLocker/ExtDisplays/%08X.ini", crc32);
    if (file_exists(path) == true) {
		FILE* file = fopen(path, "r");
		fseek(file, 0, 2);
		size_t size = ftell(file);
		fseek(file, 0, 0);
		std::string string_data(size, 0);
		fread(string_data.data(), size, 1, file);
		fclose(file);
		if (size == 0) return;
		tsl::hlp::ini::IniData iniData = tsl::hlp::ini::parseIni(string_data);
		if (iniData.contains("Common") == false) {
			return;
		}
		if (iniData["Common"].contains("tvName") == true) {
			TV_name = iniData["Common"]["tvName"];
		}
		if (!is720p) {
			if (iniData["Common"].contains("refreshRateAllowed") == false) {
				return;
			}
			if (iniData["Common"]["refreshRateAllowed"].begin()[0] != '{')
				return;
			if ((iniData["Common"]["refreshRateAllowed"].end()-1)[0] != '}')
				return;
			std::string rrAllowed = std::string(iniData["Common"]["refreshRateAllowed"].begin()+1, iniData["Common"]["refreshRateAllowed"].end()-1);
			for (const auto& word_view : std::views::split(rrAllowed, ',')) {
				std::string temp_string(word_view.begin(), word_view.end());
				int value = 0;
				auto [ptr, ec] = std::from_chars(temp_string.c_str(), &temp_string.c_str()[temp_string.length()], value);
				if (ec != std::errc{}) return;
				for (size_t i = 0; i < sizeof(DockedModeRefreshRateAllowedValues); i++) {
					if (value == DockedModeRefreshRateAllowedValues[i]) {
						rr[i] = true;
						break;
					}
				}
			}
		}
		else {
			if (iniData["Common"].contains("refreshRateAllowed720p") == false) {
				return;
			}
			if (iniData["Common"]["refreshRateAllowed720p"].begin()[0] != '{')
				return;
			if ((iniData["Common"]["refreshRateAllowed720p"].end()-1)[0] != '}')
				return;
			std::string rrAllowed = std::string(iniData["Common"]["refreshRateAllowed720p"].begin()+1, iniData["Common"]["refreshRateAllowed720p"].end()-1);
			for (const auto& word_view : std::views::split(rrAllowed, ',')) {
				std::string temp_string(word_view.begin(), word_view.end());
				int value = 0;
				auto [ptr, ec] = std::from_chars(temp_string.c_str(), &temp_string.c_str()[temp_string.length()], value);
				if (ec != std::errc{}) return;
				for (size_t i = 0; i < sizeof(DockedModeRefreshRateAllowedValues); i++) {
					if (value == DockedModeRefreshRateAllowedValues[i]) {
						rr[i] = true;
						break;
					}
				}
			}
		}
		if (iniData["Common"].contains("allowPatchesToForce60InDocked") == true) {
			as.dontForce60InDocked = (bool)!strncasecmp(iniData["Common"]["allowPatchesToForce60InDocked"].c_str(), "False", 5);
		}
		if (iniData["Common"].contains("matchLowestRefreshRate") == true) {
			as.fpsTargetWithoutRRMatchLowest = (bool)!strncasecmp(iniData["Common"]["matchLowestRefreshRate"].c_str(), "True", 4);
		}
		if (iniData["Common"].contains("bringDefaultRefreshRateWhenOutOfFocus") == true) {
			as.displaySyncDockedOutOfFocus60 = (bool)!strncasecmp(iniData["Common"]["bringDefaultRefreshRateWhenOutOfFocus"].c_str(), "True", 4);
		}
    }
    return;
}

void SaveDockedModeAllowedSave(DockedModeRefreshRateAllowed rr, DockedAdditionalSettings &as, bool is720p) {
	tsl::hlp::doWithSmSession([]{
		setsysInitialize();
	});
    SetSysEdid edid = {0};
    if (R_FAILED(setsysGetEdid(&edid))) {
		return;
    }
    char path[128] = "";
	DockedModeRefreshRateAllowed rr_impl = {0};
	DockedAdditionalSettings as_impl = {0};
	LoadDockedModeAllowedSave(rr_impl, as_impl, nullptr, !is720p);
    snprintf(path, sizeof(path), "sdmc:/SaltySD/plugins/FPSLocker/ExtDisplays/%08X.ini", crc32Calculate(&edid, sizeof(edid)));
    FILE* file = fopen(path, "w");
    if (file) {
		std::string allowedRR = "{";
		std::string allowedRR720p = "{";
		if (!is720p) {
			for (size_t i = 0; i < sizeof(DockedModeRefreshRateAllowed); i++) {
				if (rr[i]) {
					allowedRR += std::to_string(DockedModeRefreshRateAllowedValues[i]);
					allowedRR += ",";
				}
				if (rr_impl[i]) {
					allowedRR720p += std::to_string(DockedModeRefreshRateAllowedValues[i]);
					allowedRR720p += ",";
				}
			}		
		}
		else {
			for (size_t i = 0; i < sizeof(DockedModeRefreshRateAllowed); i++) {
				if (rr[i]) {
					allowedRR720p += std::to_string(DockedModeRefreshRateAllowedValues[i]);
					allowedRR720p += ",";
				}
				if (rr_impl[i]) {
					allowedRR += std::to_string(DockedModeRefreshRateAllowedValues[i]);
					allowedRR += ",";
				}
			}
		}
		allowedRR.erase(allowedRR.end()-1);
		allowedRR += "}";
		allowedRR720p.erase(allowedRR720p.end()-1);
		allowedRR720p += "}";
		fwrite("[Common]\n", strlen("[Common]\n"), 1, file);
		fwrite("tvName=", strlen("tvName="), 1, file);
		fwrite(TV_name.c_str(), TV_name.length(), 1, file);
		fwrite("\n", 1, 1, file);
		fwrite("refreshRateAllowed=", strlen("refreshRateAllowed="), 1, file);
		fwrite(allowedRR.c_str(), allowedRR.length(), 1, file);
		fwrite("\n", 1, 1, file);
		std::string df60 = (as.dontForce60InDocked ? "False" : "True");
		std::string fpst = (as.fpsTargetWithoutRRMatchLowest ? "True" : "False");
		std::string dsdo = (as.displaySyncDockedOutOfFocus60 ? "True" : "False");
		fwrite("allowPatchesToForce60InDocked=", strlen("allowPatchesToForce60InDocked="), 1, file);
		fwrite(df60.c_str(), df60.length(), 1, file);
		fwrite("\n", 1, 1, file);
		fwrite("matchLowestRefreshRate=", strlen("matchLowestRefreshRate="), 1, file);
		fwrite(fpst.c_str(), fpst.length(), 1, file);
		fwrite("\n", 1, 1, file);
		fwrite("bringDefaultRefreshRateWhenOutOfFocus=", strlen("bringDefaultRefreshRateWhenOutOfFocus="), 1, file);
		fwrite(dsdo.c_str(), dsdo.length(), 1, file);
		fwrite("\n", 1, 1, file);
		fwrite("refreshRateAllowed720p=", 23, 1, file);
		fwrite(allowedRR720p.c_str(), allowedRR720p.length(), 1, file);
		fwrite("\n", 1, 1, file);
        fclose(file);

    }
    return;
}

char expected_display_version[0x10] = "";

_Atomic(int) cancel_flag = 0;

curl_off_t data_to_download = 0;
curl_off_t data_downloaded = 0;

static int xfer_callback(void *clientp, curl_off_t dltotal, curl_off_t dlnow,
                         curl_off_t ultotal, curl_off_t ulnow) {
	data_to_download = dltotal;
	data_downloaded = dlnow;
    if (atomic_load(&cancel_flag)) {
        return 1; // Abort
    }
    return 0;
}

std::array sources = {
	std::pair<const char*, const char*>("https://gitee.com/sskyswitch/FPSLocker-Warehouse/raw/v4/", ""),
	std::pair<const char*, const char*>("https://raw.githubusercontent.com/masagrator/FPSLocker-Warehouse/v4/", "")
};

/**
 * @brief Gets the \ref NsApplicationControlData for the specified application.
 * @note Only available on [21.0.0+]. Faster than nsGetApplicationControlData2 while having the same functionality.
 * @param[in] source Source, official sw uses ::NsApplicationControlSource_Storage.
 * @param[in] application_id ApplicationId.
 * @param[out] buffer \ref NsApplicationControlData
 * @param[in] flag1 Default is 0. 0xFF speeds up execution.
 * @param[in] flag2 Default is 0.
 * @param[in] size Size of the buffer.
 * @param[out] actual_size Actual output size.
 * @param[out] unk Returned with size, always 0.
 */
Result nsGetApplicationControlData3(NsApplicationControlSource source, u64 application_id, NsApplicationControlData* buffer, size_t size, u8 flag1, u8 flag2, u64* actual_size) {
    Service srv={0}, *srv_ptr = &srv;
    Result rc=0;
    u32 cmd_id = 20;
    rc = nsGetReadOnlyApplicationControlDataInterface(&srv);

    const struct {
        u8 source;
        u8 flags[2];
        u8 pad[5];
        u64 application_id;
    } in = { source, {flag1, flag2}, {0}, application_id };

    struct {
		u32 unk1;
		u32 size;
		u32 unk2;
	} tmp;

    if (R_SUCCEEDED(rc)) rc = serviceDispatchInOut(srv_ptr, cmd_id, in, tmp,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { buffer, size } },
    );
    if (R_SUCCEEDED(rc)) {
        if (actual_size) *actual_size = tmp.size;
    }

    serviceClose(&srv);
    return rc;
}

/**
 * @brief GetApplicationTitle. Returns \ref NacpLanguageEntry matching currently set system language for each specified ApplicationId.
 * @note The data available with \ref asyncValueGet is a s32 for the offset within the buffer where the output data is located, \ref asyncValueGetSize returns the total byte-size of the data located here. The data located here is the \ref NacpLanguageEntry for each specified ApplicationId.
 * @note Only available on [20.0.0+].
 * @note NacpLanguageEntry is decompressed when necessary only on [21.0.0+].
 * @param[out] a \ref AsyncValue
 * @param[in] source Source, official sw uses ::NsApplicationControlSource_Storage.
 * @param[in] application_ids Input array of ApplicationIds.
 * @param[in] count Size of the application_ids array in entries.
 * @param buffer 0x1000-byte aligned buffer for TransferMemory. This buffer must not be accessed until the async operation finishes.
 * @param[in] size 0x1000-byte aligned buffer size for TransferMemory. This must be at least: count*sizeof(\ref NacpLanguageEntry) + count*sizeof(u64) + sizeof(\ref NsApplicationControlData).
 */
Result nsGetApplicationTitle(AsyncValue *a, NsApplicationControlSource source, const u64 *application_ids, s32 count, void* buffer, size_t size) {
    if (hosversionBefore(20,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    Result rc=0;
    TransferMemory tmem={0};
	Service srv={0};
	u32 cmd_id = 10;

    rc = tmemCreateFromMemory(&tmem, buffer, size, Perm_R);
    if (R_FAILED(rc))
		return rc;

	rc = nsGetReadOnlyApplicationControlDataInterface(&srv);

	if (R_SUCCEEDED(rc)) {
		const struct {
			u8 source;
			u8 pad[7];
			u64 size;
		} in = { source, {0}, tmem.size };

		memset(a, 0, sizeof(*a));
		Handle event = INVALID_HANDLE;
		rc = serviceDispatchIn(&srv, cmd_id, in,
			.buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
			.buffers = { { application_ids, count*sizeof(u64) } },
			.in_num_handles = 1,
			.in_handles = { tmem.handle },
			.out_num_objects = 1,
			.out_objects = &a->s,
			.out_handle_attrs = { SfOutHandleAttr_HipcCopy },
			.out_handles = &event,
		);

		if (R_SUCCEEDED(rc))
			eventLoadRemote(&a->event, event, false);

		serviceClose(&srv);
	}

	tmemClose(&tmem);

    return rc;
}

void sendConfirmation(Result temp_error_code) {
	s32 appContentMetaStatusSize = 0;
	NsApplicationControlData* appControlData = new NsApplicationControlData;
	NsApplicationContentMetaStatus* appContentMetaStatus = new NsApplicationContentMetaStatus[2];
	char display_version[sizeof(appControlData -> nacp.display_version)] = "";
	uint32_t version = 0;
	Result rc = 1;
	if (hosversionBefore(19,0,0)) {
		rc = nsGetApplicationControlData(NsApplicationControlSource_Storage, TID, appControlData, sizeof(NsApplicationControlData), nullptr);
	}
	else if (hosversionBefore(21,0,0)) {
		rc = nsGetApplicationControlData2(NsApplicationControlSource_Storage, TID, appControlData, sizeof(NsApplicationControlData), 0xFF, 0, nullptr, nullptr);
	}
	else rc = nsGetApplicationControlData3(NsApplicationControlSource_Storage, TID, appControlData, sizeof(NsApplicationControlData), 0xFF, 0, nullptr);

	if (R_SUCCEEDED(rc)) {
		strcpy(display_version, appControlData->nacp.display_version);
		if (R_SUCCEEDED(nsListApplicationContentMetaStatus(TID, 0, appContentMetaStatus, 2, &appContentMetaStatusSize))) {
			u32 index = 0;
			if (appContentMetaStatusSize == 2 && appContentMetaStatus[1].meta_type == NcmContentMetaType_Patch) index = 1;
			version = appContentMetaStatus[index].version / 65536;
		}
	}
	delete appControlData;
	delete[] appContentMetaStatus;

	static uint64_t last_TID_checked = 0;
	if (TID != last_TID_checked) {
		last_TID_checked = TID;
		CURL *curl_ga = curl_easy_init();
		if (curl_ga) {
			const char macro_id[] = "\x41\x4B\x66\x79\x63\x62\x78\x72\x77\x45\x30\x51\x66\x75\x39\x34\x4A\x38\x44\x6E\x69\x53\x46\x6A\x33\x61\x73\x73\x6C\x68\x78\x42\x46\x43\x2D\x50\x52\x7A\x50\x64\x55\x6E\x37\x41\x5F\x4C\x4D\x61\x69\x37\x4F\x56\x57\x42\x70\x6E\x62\x73\x61\x53\x77\x55\x4D\x42\x72\x44\x69\x45\x69\x6F\x57\x65\x33\x77";
			const char m_template[] = "\x68\x74\x74\x70\x73\x3a\x2f\x2f\x73\x63\x72\x69\x70\x74\x2e\x67\x6f\x6f\x67\x6c\x65\x2e\x63\x6f\x6d\x2f\x6d\x61\x63\x72\x6f\x73\x2f\x73\x2f\x25\x73\x2f\x65\x78\x65\x63\x3f\x54\x49\x44\x3d\x25\x30\x31\x36\x6c\x58\x26\x42\x49\x44\x3d\x25\x30\x31\x36\x6c\x58\x26\x56\x65\x72\x73\x69\x6f\x6e\x3d\x25\x64\x26\x44\x69\x73\x70\x6c\x61\x79\x56\x65\x72\x73\x69\x6f\x6e\x3d\x25\x73\x26\x46\x6f\x75\x6e\x64\x3d\x25\x64\x26\x4e\x52\x4f\x3d\x25\x30\x31\x36\x6c\x58\x26\x41\x70\x70\x56\x65\x72\x73\x69\x6f\x6e\x3d\x25\x73";
			char link[256] = "";
			MemoryInfo mem = {0};
			u32 pageinfo = 0;
			svcQueryMemory(&mem, &pageinfo, (uintptr_t)&file_exists);

			char* display_version_converted = curl_easy_escape(curl_ga, display_version, 0);
			char* app_version_converted = curl_easy_escape(curl_ga, APP_VERSION, 0);
			uint8_t valid = 1;
			if (temp_error_code == 0x404) valid = 0;
			else if (temp_error_code == 0x312) valid = 2;
			else valid = 3;
			snprintf(link, sizeof(link), m_template, macro_id, TID, BID, version, display_version_converted, valid, *(uint64_t*)(mem.addr + 64), APP_VERSION);
			curl_free(display_version_converted);
			curl_free(app_version_converted);

			curl_easy_setopt(curl_ga, CURLOPT_URL, link);
			curl_easy_setopt(curl_ga, CURLOPT_SSL_VERIFYPEER, 0L);
			curl_easy_setopt(curl_ga, CURLOPT_SSL_VERIFYHOST, 0L);
			curl_easy_setopt(curl_ga, CURLOPT_TIMEOUT_MS, 1000);
			curl_easy_perform(curl_ga);
			curl_easy_cleanup(curl_ga);
		}
	}
}

#define timeout_in_seconds 30

Result downloadPatchImpl(const char* source, const char* suffix) {


	Result temp_error_code = -1;

	curl_timeout = false;

	uint64_t startTick = svcGetSystemTick();
	uint64_t timeoutTick = startTick + (timeout_in_seconds * systemtickfrequency);
	long msPeriod = (timeoutTick - svcGetSystemTick()) / (systemtickfrequency / 1000);

	CURL *curl = curl_easy_init();

    if (curl) {

		char download_path[256] = "";
		char file_path[192] = "";
		snprintf(download_path, sizeof(download_path), "sdmc:/SaltySD/plugins/FPSLocker/patches/%016lX/", TID);
		
		std::filesystem::create_directories(download_path);

		snprintf(file_path, sizeof(file_path), "sdmc:/SaltySD/plugins/FPSLocker/patches/%016lX/temp.yaml", TID);

		FILE* fp = fopen(file_path, "wb+");
		if (!fp) {
			curl_easy_cleanup(curl);
			return 0x101;
		}

		//FILE* logfileerr = fopen("sdmc:/log_err.txt", "ab");

		snprintf(download_path, sizeof(download_path), "%sSaltySD/plugins/FPSLocker/patches/%016lX/%016lX.yaml%s", source, TID, BID, suffix);
        curl_easy_setopt(curl, CURLOPT_URL, download_path);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0");
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_NOBODY, 0L);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
		curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, xfer_callback);
		//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
		//curl_easy_setopt(curl, CURLOPT_STDERR, logfileerr);
		msPeriod = (timeoutTick - svcGetSystemTick()) / (systemtickfrequency / 1000);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, msPeriod);

        CURLcode res = curl_easy_perform(curl);
		
		fclose(fp);
		//fclose(logfileerr);
		if (res != CURLE_OK) {
			remove(file_path);
			if (res == CURLE_OPERATION_TIMEDOUT) temp_error_code = 0x316;
			else temp_error_code = 0x200 + res;
		}
		else {
			long http_code = 0;
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
			if (http_code == 200) {
				temp_error_code = 0;
			}
			else if (http_code == 404 || http_code == 400) {
				temp_error_code = 0x404;
			}
			else temp_error_code = 0x312;
			if (temp_error_code) remove(file_path);
		}

		if (!temp_error_code) {
			fp = fopen(file_path, "rb");
			if (!fp || fseek(fp, 0, SEEK_END) || ftell(fp) <= 0 || ftell(fp) > 32768) {
				temp_error_code = 0x312;
				if (fp) fclose(fp);
				remove(file_path);
			} else {
				FILE* old = fopen(configPath, "rb");
				const bool unchanged = download_files::equal(fp, old);
				if (old) fclose(old);
				fclose(fp);
				if (unchanged) {
					temp_error_code = 0x104;
					remove(file_path);
				} else FileDownloaded = true;
			}
			if (!temp_error_code) {
				if (rename(file_path, configPath)) {
					curl_easy_cleanup(curl);
					return 0x101;
				}
				FILE* config = fopen(configPath, "r");
				if (!config) { curl_easy_cleanup(curl); return 0x101; }
				memset(&LOCK::configBuffer, 0, sizeof(LOCK::configBuffer));
				fread(&LOCK::configBuffer, 1, 32768, config);
				const bool readFailed = ferror(config);
				fclose(config);
				if (readFailed) { curl_easy_cleanup(curl); return 0x101; }
				strcat(&LOCK::configBuffer[0], "\n");
				LOCK::tree = ryml::parse_in_place(LOCK::configBuffer);
				size_t root_id = LOCK::tree.root_id();
				if (LOCK::tree.is_map(root_id) && LOCK::tree.find_child(root_id, "Addons") != c4::yml::NONE && !LOCK::tree["Addons"].is_keyval() && LOCK::tree["Addons"].num_children() > 0) {
					for (size_t i = 0; i < LOCK::tree["Addons"].num_children(); i++) {
						std::string temp = "";
						LOCK::tree["Addons"][i] >> temp;
						std::string dpath = source + temp;
						std::string path = "sdmc:/" + temp;
						strncpy(&download_path[0], dpath.c_str(), 255);
						strncpy(&file_path[0], path.c_str(), 191);
						curl_easy_setopt(curl, CURLOPT_URL, download_path);
						msPeriod = (timeoutTick - svcGetSystemTick()) / (systemtickfrequency / 1000);
						if (msPeriod < 1000) msPeriod = 1000;
						curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, msPeriod);
						FILE* fp = fopen(file_path, "wb");
						if (!fp) {
							std::filesystem::create_directories(std::filesystem::path(file_path).parent_path());
							fp = fopen(file_path, "wb");
						}
						if (fp) {
							curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
							res = curl_easy_perform(curl);
							fclose(fp);
						}
					}
				}
			}
		}
		else if (temp_error_code == 0x404) {
			error_code = 0x404;
			std::string readme_path = source;
			readme_path += "README.md";
			readme_path += suffix;
			curl_easy_setopt(curl, CURLOPT_URL, readme_path.c_str());
			data_to_download = 0;
			data_downloaded = 0;
			fp = fopen("sdmc:/SaltySD/plugins/FPSLocker/patches/README.md", "wb+");
			if (!fp) {
				curl_easy_cleanup(curl);
				return 0x101;
			}
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
			msPeriod = (timeoutTick - svcGetSystemTick()) / (systemtickfrequency / 1000);
			if (msPeriod < 1000) msPeriod = 1000;
			curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, msPeriod);
			CURLcode res = curl_easy_perform(curl);
			if (res == CURLE_OK) {
				long http_code = 0;
				curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
				if (http_code != 200) {
					fclose(fp);
					remove("sdmc:/SaltySD/plugins/FPSLocker/patches/README.md");
					curl_easy_cleanup(curl);
					return 0x406;
				}
				temp_error_code = download_files::warehouseStatus(fp, TID, BID,
					expected_display_version, sizeof(expected_display_version));
			}
			else if (res == CURLE_OPERATION_TIMEDOUT) {
				temp_error_code = 0x405;
			}
			else temp_error_code = 0x406;
			fclose(fp);
			remove("sdmc:/SaltySD/plugins/FPSLocker/patches/README.md");
		}

        curl_easy_cleanup(curl);
    }

	return temp_error_code;
}

void updateErrorcode(Result rc, Result* last_error_code, Result* last_bad_error_code) {
	if (rc > 0 && rc < 0x1000) {
		*last_bad_error_code = rc;
	}
	else *last_error_code = rc;
}

void downloadPatch(void*) {

	if (!TID || !BID) {
		error_code = 0x316;
		return;
	}

	static const SocketInitConfig socketInitConfig = {

        .tcp_tx_buf_size = 0x800,
        .tcp_rx_buf_size = 0x8000,
        .tcp_tx_buf_max_size = 0x4000,
        .tcp_rx_buf_max_size = 0x20000,

        .udp_tx_buf_size = 0,
        .udp_rx_buf_size = 0,

        .sb_efficiency = 1,
		.bsd_service_type = BsdServiceType_Auto
    };

	if (R_FAILED(smInitialize())) { error_code = 0x101; return; }
	if (R_FAILED(nifmInitialize(NifmServiceType_System))) {
		smExit(); error_code = 0x412; return;
	}
	u32 dummy = 0;
	NifmInternetConnectionType NifmConnectionType = (NifmInternetConnectionType)-1;
	NifmInternetConnectionStatus NifmConnectionStatus = (NifmInternetConnectionStatus)-1;
	if (R_FAILED(nifmGetInternetConnectionStatus(&NifmConnectionType, &dummy, &NifmConnectionStatus)) || NifmConnectionStatus != NifmInternetConnectionStatus_Connected) {
		nifmExit();
		smExit();
		error_code = 0x412;
		return;
	}
	nifmExit();
	if (R_FAILED(socketInitialize(&socketInitConfig))) {
		smExit(); error_code = 0x101; return;
	}
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
		socketExit(); smExit(); error_code = 0x101; return;
	}

	Result last_error_code = UINT32_MAX;
	Result last_bad_error_code = 0x316;
	bool exitImmediately = false;
	for (size_t i = 0; i < sources.size(); i++) {
		Result rc = downloadPatchImpl(sources[i].first, sources[i].second);
		if (atomic_load(&cancel_flag)) {
			last_error_code = 0x312;
			exitImmediately = true;
			break;
		}
		updateErrorcode(rc, &last_error_code, &last_bad_error_code);
		if (R_SUCCEEDED(rc)) break;
	}
	if (!exitImmediately) sendConfirmation(last_error_code);
	if (last_error_code == UINT32_MAX) last_error_code = last_bad_error_code;
    curl_global_cleanup();
	socketExit();
	smExit();
	error_code = last_error_code;
	return;
}

void loopThread(void*) {
	do {
		if (R_FAILED(pmdmntGetApplicationProcessId(&PID))) break;
	} while(!leventWait(&threadexit, 1'000'000'000));
	PluginRunning = false;
	check = false;
	closed = true;
}

uint64_t getBID() {
	u64 BID_temp = 0;

	if (R_SUCCEEDED(ldrDmntInitialize())) {
		LoaderModuleInfo* module_infos = (LoaderModuleInfo*)malloc(sizeof(LoaderModuleInfo) * 16);
		if (!module_infos) { ldrDmntExit(); return 0; }
		s32 module_infos_count = 0;
		Result ret = ldrDmntGetProcessModuleInfo(PID, module_infos, 16, &module_infos_count);
		ldrDmntExit();

		if (R_SUCCEEDED(ret)) {
			for (int itr = 0; itr < module_infos_count; itr++) {
				static u64 comp_address = 0;
				if (!comp_address) {
					comp_address = module_infos[itr].base_address;
					continue;
				}
				if ((module_infos[itr].base_address - comp_address == 0x4000) || (module_infos[itr].base_address - comp_address == 0x6000) || (module_infos[itr].base_address - comp_address == 0x5000)) {
					for (int itr2 = 0; itr2 < 8; itr2++) {
						*(uint8_t*)((uint64_t)&BID_temp+itr2) = module_infos[itr].build_id[itr2];
					}
					BID_temp = __builtin_bswap64(BID_temp);
					itr = module_infos_count;
				}
				else comp_address = module_infos[itr].base_address;
			}
		}
		free(module_infos);
	}

	return BID_temp;
}

bool LoadSharedMemory() {
	if (SaltySD_Connect())
		return false;

	SaltySD_GetSharedMemoryHandle(&remoteSharedMemory);
	SaltySD_Term();

	shmemLoadRemote(&_sharedmemory, remoteSharedMemory, 0x1000, Perm_Rw);
	if (!shmemMap(&_sharedmemory)) {
		SharedMemoryUsed = true;
		return true;
	}
	return false;
}

ptrdiff_t searchSharedMemoryBlock(uintptr_t base) {
	ptrdiff_t search_offset = 0;
	while(search_offset < 0x1000) {
		uint32_t* MAGIC_shared = (uint32_t*)(base + search_offset);
		if (*MAGIC_shared == 0x465053) {
			return search_offset;
		}
		else search_offset += 4;
	}
	return -1;
}

bool CheckPort () {
	Handle saltysd;
	for (int i = 0; i < 67; i++) {
		if (R_SUCCEEDED(svcConnectToNamedPort(&saltysd, "InjectServ"))) {
			svcCloseHandle(saltysd);
			break;
		}
		else {
			if (i == 66) return false;
			svcSleepThread(1'000'000);
		}
	}
	for (int i = 0; i < 67; i++) {
		if (R_SUCCEEDED(svcConnectToNamedPort(&saltysd, "InjectServ"))) {
			svcCloseHandle(saltysd);
			return true;
		}
		else svcSleepThread(1'000'000);
	}
	return false;
}

// Returns true if decompressed correctly.
bool nacp_decompress(NsApplicationControlData* appControlData)
{	
	
	NacpStruct2* nacp = (NacpStruct2*)&(appControlData -> nacp);
	Bytef* temp_buffer = (Bytef*)calloc(32, sizeof(NacpLanguageEntry));
	if (!temp_buffer)
		return false;

    z_stream strm = {0};

    if (inflateInit2(&strm, -15) != Z_OK) {
		free(temp_buffer);
        return false;
	}

    strm.next_in   = (Bytef*)nacp->lang_data.compressed_data.buffer;
    strm.avail_in  = nacp->lang_data.compressed_data.buffer_size;
    strm.next_out  = temp_buffer;
    strm.avail_out = sizeof(NacpLanguageEntry[32]);

    int ret = inflate(&strm, Z_FINISH);
	bool res = false;
    if (ret == Z_STREAM_END) {
		memcpy(&appControlData->nacp, temp_buffer, 0x3000);
		res = true;
	}

    inflateEnd(&strm);
	free(temp_buffer);
    return res;
}

std::string getAppName(uint64_t Tid)
{
	NsApplicationControlData* appControlData = (NsApplicationControlData*)malloc(sizeof(NsApplicationControlData));

	if (!appControlData) {
		char fallback[17];
		snprintf(fallback, sizeof(fallback), "%016lX", Tid);
		return fallback;
	}
	Result rc = -1;
	if (hosversionBefore(19,0,0)) {
		rc = nsGetApplicationControlData(NsApplicationControlSource::NsApplicationControlSource_Storage, Tid, appControlData, sizeof(NsApplicationControlData), nullptr);
	}
	//This is faster by 30% than function above on 19.0.0-20.5.0
	else if (hosversionBefore(21,0,0)) {
		rc = nsGetApplicationControlData2(NsApplicationControlSource::NsApplicationControlSource_Storage, Tid, appControlData, sizeof(NsApplicationControlData), 0xFF, 0, nullptr, nullptr);
	}
	//This is faster by 10% than function above on 21.0.0-21.2.0
	else if (hosversionBefore(22,0,0)) {
		rc = nsGetApplicationControlData3(NsApplicationControlSource::NsApplicationControlSource_Storage, Tid, appControlData, sizeof(NsApplicationControlData), 0xFF, 0, nullptr);
	}
	if (R_FAILED(rc)) {
		free(appControlData);
		char returnTID[18];
		sprintf(returnTID, "%016lx-", Tid);
		return (std::string)returnTID;
	}

	NacpStruct2* nacp = (NacpStruct2*)&(appControlData -> nacp);
	if (nacp->titles_data_format == 1) {
		if (!nacp_decompress(appControlData)) {
			free(appControlData);
			char returnTID[18];
			sprintf(returnTID, "%016lx-", Tid);
			return (std::string)returnTID;			
		}
	}
	
	NacpLanguageEntry* languageEntry = &nacp->lang_data.lang[getNacpLanguage()];
	if (languageEntry->name[0] == 0) {
		for (size_t i = 0; i < 16; i++) {
			if (nacp->lang_data.lang[i].name[0] != 0) {
				languageEntry = &nacp->lang_data.lang[i];
				break;
			}
		}
	}
	if (languageEntry->name[0] == 0) {
		free(appControlData);
		char returnTID[18];
		sprintf(returnTID, "0x%X", rc);
		return (std::string)returnTID;
	}
	std::string to_return = languageEntry->name;
	free(appControlData);
	return to_return;
}

Result getTitles(int32_t count)
{
	if (count <= 0) return 0;
	NsApplicationRecord* appRecords = (NsApplicationRecord*)malloc(count * sizeof(NsApplicationRecord));
	if (!appRecords) return 0x101;
	int32_t actualAppRecordCnt = 0;
	Result rc = nsListApplicationRecord(appRecords, count, 0, &actualAppRecordCnt);
	if (R_FAILED(rc)) {
		free(appRecords);
		return rc;
	}
	if (actualAppRecordCnt <= 0) { free(appRecords); return 0; }
	uint64_t* TIDs = (uint64_t*)malloc(actualAppRecordCnt*sizeof(uint64_t));
	if (!TIDs) { free(appRecords); return 0x101; }
	for (int32_t i = 0; i < actualAppRecordCnt; i++) {
		TIDs[i] = appRecords[i].application_id;
	}
	free(appRecords);
	if (hosversionBefore(22,0,0)) for (int32_t i = 0; i < actualAppRecordCnt; i++) {
		uint64_t m_TID = TIDs[i];
		if (m_TID != 0) {
			Title title;
			title.TitleID = m_TID;
			title.TitleName = getAppName(m_TID);
			mutexLock(&TitlesAccess);
			titles.emplace_back(title);
			mutexUnlock(&TitlesAccess);
		}
	}
	else {
		size_t size = ((actualAppRecordCnt * 0x308) + sizeof(NsApplicationControlData) + 0x1000) & ~0xFFF;
		void* buffer = aligned_alloc(0x1000, size);
		if (!buffer) { free(TIDs); return 0x101; }
		AsyncValue _asyncValue;
		rc = nsGetApplicationTitle(&_asyncValue, NsApplicationControlSource_Storage, TIDs, actualAppRecordCnt, buffer, size);
		if (R_FAILED(rc)) {
			free(TIDs);
			free(buffer);
			return rc;
		}
		s32 offset;
		rc = asyncValueGet(&_asyncValue, &offset, sizeof(offset));
		if (R_FAILED(rc)) {
			free(TIDs);
			free(buffer);
			return rc;
		}
		NacpLanguageEntry* language_data = (NacpLanguageEntry*)(uintptr_t(buffer) + offset);
		for (int32_t i = 0; i < actualAppRecordCnt; i++) {
			if (language_data[i].name[0] == 0)
				continue;
			Title title;
			title.TitleID = TIDs[i];
			title.TitleName = language_data[i].name;
			mutexLock(&TitlesAccess);
			titles.emplace_back(title);
			mutexUnlock(&TitlesAccess);
		}
		asyncValueClose(&_asyncValue);
		free(buffer);
	}
	free(TIDs);
	return rc;
}

void TitlesThread(void*) {
	getTitles(32);
}

void setForceEnglishLanguage(bool set) {
	uintptr_t ptr_func = (uintptr_t)&TitlesThread;
	MemoryInfo mem = {0};
	u32 pageinfo = 0;
	svcQueryMemory(&mem, &pageinfo, ptr_func);
	bool* ptrBool = (bool*)&forceEnglishLanguage;
	uintptr_t ptrBool_integer = (uintptr_t)ptrBool;
	ptrdiff_t ptrBool_offset = ptrBool_integer - mem.addr;
	FILE* file = fopen(overlayName.c_str(), "rb+");
	if (file) {
		fseek(file, ptrBool_offset, 0);
		fwrite(&set, 1, 1, file);
		fclose(file);
	}
}

bool saveSettings() {
	if (!(Shared -> FPSlocked) && !(Shared -> FPSlockedDocked) && !(Shared -> ZeroSync) && !SetBuffers_save && !forceSuspend_save) {
		remove(savePath);
	}
	else {
		DIR* dir = opendir("sdmc:/SaltySD/plugins/");
		if (!dir) {
			mkdir("sdmc:/SaltySD/plugins/", 777);
		}
		else closedir(dir);
		dir = opendir("sdmc:/SaltySD/plugins/FPSLocker/");
		if (!dir) {
			mkdir("sdmc:/SaltySD/plugins/FPSLocker/", 777);
		}
		else closedir(dir);
		FILE* file = fopen(savePath, "wb");
		if (file) {
			fwrite(&(Shared->FPSlocked), 1, 1, file);
			if (SetBuffers_save > 2 || (!SetBuffers_save && (Shared -> Buffers) > 2)) {
				(Shared -> ZeroSync) = 0;
			}
			fwrite(&(Shared->ZeroSync), 1, 1, file);
			fwrite(&SetBuffers_save, 1, 1, file);
			fwrite(&forceSuspend_save, 1, 1, file);
			fwrite(&(Shared->FPSlockedDocked), 1, 1, file);
			fclose(file);
		}
		else return false;
	}
	return true;


}
