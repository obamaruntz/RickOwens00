#pragma once

#define win_1803 17134
#define win_1809 17763
#define win_1903 18362
#define win_1909 18363
#define win_2004 19041
#define win_20H2 19569
#define win_21H1 20180
#define win_22H2 19045

// 用于获取任何 Windows 版本中的用户目录偏移量
SHORT get_windows_version() {
	RTL_OSVERSIONINFOW windows_version = {};
	RtlGetVersion(&windows_version);

	switch (windows_version.dwBuildNumber) {
	case win_1803:
	case win_1809:
		return 0x0278;
		break;
	case win_1903:
	case win_1909:
		return 0x0280;
		break;
	case win_2004:
	case win_20H2:
	case win_21H1:
	case win_22H2:
		return 0x0388;
		break;
	default:
		return 0x0388;
	}
}