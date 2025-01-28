#include "stdafx.h"
#include "resource.h"

#include <foobar2000/SDK/coreDarkMode.h>

namespace {
// {E7E34E73-F17E-4c30-8FBE-435C649060C5}
static const GUID guid_preferences_page1 = { 0xe7e34e73, 0xf17e, 0x4c30, { 0x8f, 0xbe, 0x43, 0x5c, 0x64, 0x90, 0x60, 0xc5 } };
// {21CB8CF4-0332-4d57-9740-C8443E2B873B}
static const GUID guid_preferences_page2 = { 0x21cb8cf4, 0x332, 0x4d57, { 0x97, 0x40, 0xc8, 0x44, 0x3e, 0x2b, 0x87, 0x3b } };

class preferences_page_main : public preferences_page_instance, public CDialogImpl<preferences_page_main>
{
private:
	fb2k::CCoreDarkModeHooks m_hooks;

	config_main cfg_main_new;

	void update_credentials() {
		GetDlgItem(IDC_CONTROL_PASSWORD).EnableWindow(cfg_main_new.control_credentials);
		GetDlgItem(IDC_CONTROL_USERNAME).EnableWindow(cfg_main_new.control_credentials);
		GetDlgItem(IDC_STATIC_PASSWORD).EnableWindow(cfg_main_new.control_credentials);
		GetDlgItem(IDC_STATIC_USERNAME).EnableWindow(cfg_main_new.control_credentials);
	}

	void update_server_status()	{
		if (!httpc::control::listener_error)
		{
			uSetDlgItemText(m_hWnd, IDC_LAUNCH_BROWSER, pfc::string_formatter() << "Open " << httpc::control::listener_info);
			GetDlgItem(IDC_LAUNCH_BROWSER).EnableWindow(true);
		}
		if (httpc::control::listener_error)
		{
			SetDlgItemText(IDC_LAUNCH_BROWSER, _T("Bind failed"));
			GetDlgItem(IDC_LAUNCH_BROWSER).EnableWindow(false);
		}
		if (!cfg_main_new.startserver)
		{
			SetDlgItemText(IDC_LAUNCH_BROWSER, _T(""));
			GetDlgItem(IDC_LAUNCH_BROWSER).EnableWindow(false);
		}
	}

	void updateDialog()	{
		uSetDlgItemText(m_hWnd, IDC_INTERFACE, cfg_main_new.ip);
		SetDlgItemInt(IDC_PORT, (unsigned int)cfg_main_new.port);
		CheckDlgButton(IDC_SERVERSTARTED, cfg_main_new.startserver);
		CheckDlgButton(IDC_HIDE_NONPLAYABLES, cfg_main_new.hide_nonplayables);
		CheckDlgButton(IDC_LOG_ACCESS, cfg_main_new.log_access);
		uSetDlgItemText(m_hWnd, IDC_CONTROL_IP, cfg_main_new.control_ip);
		CheckDlgButton(IDC_CONTROL_CREDENTIALS, cfg_main_new.control_credentials);
		uSetDlgItemText(m_hWnd, IDC_CONTROL_USERNAME, cfg_main_new.control_credentials_username);
		uSetDlgItemText(m_hWnd, IDC_CONTROL_PASSWORD, cfg_main_new.control_credentials_password);
		uSetDlgItemText(m_hWnd, IDC_CONTROL_PATH, cfg_main_new.restrict_to_path);
		uSetDlgItemText(m_hWnd, IDC_EXTRA_FORMATS, cfg_main_new.extra_formats);
		uSetDlgItemText(m_hWnd, IDC_IGNORED_FORMATS, cfg_main_new.ignored_formats);
		uSetDlgItemText(m_hWnd, IDC_ALLOWED_PROTOCOLS, cfg_main_new.allowed_protocols);
		uSetDlgItemText(m_hWnd, IDC_SERVER_ROOT, cfg_main_new.server_root);
		CheckDlgButton(IDC_STOP_AFTER_QUEUE_ENABLE, cfg_main_new.stop_after_queue_enable);
		CheckDlgButton(IDC_GZIP_ENABLE, cfg_main_new.gzip_enable);
	}

	bool is_server_root_valid()	{
		bool result = false;

		if (cfg_main_new.server_root.get_length())
		{
			try
			{
				foobar2000_io::abort_callback_dummy abort_c;
				result = foobar2000_io::filesystem::g_exists(cfg_main_new.server_root, abort_c); // todo tofix g_exists
			}
			catch (...)
			{
			}
		}
		else
			result = true;

		return result;
	}

	void set_server_root_btn_state(bool state) {
		GetDlgItem(IDC_SERVER_ROOT_BTN).EnableWindow(state);
	}

public:
	enum { IDD = IDD_TAB1 };

	preferences_page_main(preferences_page_callback::ptr callback) : m_callback(callback) { cfg_main_new = cfg.main; }

	t_uint32 get_state() {
		t_uint32 state = preferences_state::resettable;
		if (!(cfg.main == cfg_main_new)) state |= preferences_state::changed;
		return state | preferences_state::dark_mode_supported;
	}

	void apply() {
		if (httpc::control::running_threads == 0)
		{
			cfg.main = cfg_main_new;

			httpc::choose_srv_home_dir();
			httpc::control_credentials_auth_hash_update();
			httpc::get_registered_extensions();
			httpc::set_allowed_protocols();
			httpc::build_restrict_to_path_list();

			if (cfg.main.startserver != httpc::control::is_active())
				httpc::control::set_active(cfg.main.startserver);

			update_server_status();

			httpc::should_update_playlist = true;
			httpc::should_update_queue = true;

			if (!cfg.main.stop_after_queue_enable)
				cfg.misc.stop_after_queue = false;

			onChanged();
		}
		else
		{
			popup_message::g_show(pfc::string_formatter() << "Client request proceesing is active, settings not saved.\nPlease press Apply when request is processed.",
				"Warning", popup_message::icon_error);
		}
	}

	void reset() {
		cfg_main_new.reset();

		updateDialog();
		update_credentials();
		update_server_status();

		onChanged();
	}

	HWND get_wnd() { return m_hWnd; }

	BEGIN_MSG_MAP_EX(preferences_page_main)
		MSG_WM_INITDIALOG(OnInitDialog)
		MSG_WM_COMMAND(OnCommand)
		END_MSG_MAP()
private:
	BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam) {
		m_hooks.AddDialogWithControls(*this);

		updateDialog();
		update_credentials();
		update_server_status();

		set_server_root_btn_state(is_server_root_valid());

		return TRUE;
	}

	void OnCommand(UINT uNotifyCode, int nID, CWindow wndCtl) {
		switch(nID | (uNotifyCode << 16))
		{
			case IDC_INTERFACE | (EN_CHANGE << 16):
				cfg_main_new.ip = uGetDlgItemText(*this, nID);
				break;
			case IDC_PORT | (EN_CHANGE << 16):
				cfg_main_new.port = GetDlgItemInt(nID);
				break;
			case IDC_SERVERSTARTED | BN_CLICKED:
				cfg_main_new.startserver = IsDlgButtonChecked(nID) == BST_CHECKED;
				break;
			case IDC_SERVER_ROOT_BTN | (BN_CLICKED << 16): {
					pfc::string8 dir_to_open = cfg_main_new.server_root.get_length() == 0 ? httpc::srv_home_dir : cfg_main_new.server_root;
					pfc::stringcvt::string_wide_from_utf8 path_w(dir_to_open);
					ShellExecute(NULL, _T("explore"), path_w, NULL, NULL, SW_SHOWNORMAL);
				}
				break;
			case IDC_HIDE_NONPLAYABLES | (BN_CLICKED << 16):
				cfg_main_new.hide_nonplayables = IsDlgButtonChecked(nID) == BST_CHECKED;
				break;
			case IDC_LOG_ACCESS | (BN_CLICKED << 16):
				cfg_main_new.log_access = IsDlgButtonChecked(nID) == BST_CHECKED;
				break;
			case IDC_SERVER_ROOT | (EN_CHANGE << 16):
				cfg_main_new.server_root = trim(uGetDlgItemText(*this, nID));
				set_server_root_btn_state(is_server_root_valid());
				break;
			case IDC_SERVER_ROOT | (EN_KILLFOCUS << 16):
				if (!is_server_root_valid()) {
					popup_message::g_show(pfc::string_formatter() << "It appears the home directory path you entered, " << cfg_main_new.server_root
											<< ", is invalid. \n\nHome directory specifies the path foo_httpcontrol uses to look for templates.\nWhen left blank, it defaults to %APPDATA%\\foobar2000\\foo_httpcontrol if foobar2000 is installed in standard mode, or %FB2K_DIRECTORY%\\foo_httpcontrol if foobar2000 is installed in portable mode.",
											"Home directory error", popup_message::icon_error);
					set_server_root_btn_state(false);
				}
				else
					set_server_root_btn_state(true);
				break;
			case IDC_LAUNCH_BROWSER | (BN_CLICKED << 16):
				{
					pfc::stringcvt::string_wide_from_utf8 path_w(httpc::control::listener_info);
					ShellExecute(NULL, _T("open"), path_w, NULL, NULL, SW_SHOWNORMAL);
				}
				break;
			case IDC_CONTROL_IP | (EN_CHANGE << 16):
				cfg_main_new.control_ip = uGetDlgItemText(*this, nID);
				break;
			case IDC_CONTROL_CREDENTIALS | (BN_CLICKED << 16):
				cfg_main_new.control_credentials = IsDlgButtonChecked(nID) == BST_CHECKED;
				update_credentials();
				break;
			case IDC_CONTROL_PASSWORD | (EN_CHANGE << 16):
				cfg_main_new.control_credentials_password = trim(uGetDlgItemText(*this, nID));
				break;
			case IDC_CONTROL_USERNAME | (EN_CHANGE << 16):
				cfg_main_new.control_credentials_username = trim(uGetDlgItemText(*this, nID));
				break;
			case IDC_CONTROL_PATH | (EN_CHANGE << 16):
				cfg_main_new.restrict_to_path = trim(uGetDlgItemText(*this, nID));
				break;
			case IDC_EXTRA_FORMATS | (EN_CHANGE << 16):
				cfg_main_new.extra_formats = trim(uGetDlgItemText(*this, nID));
				break;
			case IDC_IGNORED_FORMATS | (EN_CHANGE << 16) :
				cfg_main_new.ignored_formats = trim(uGetDlgItemText(*this, nID));
				break;
			case IDC_ALLOWED_PROTOCOLS | (EN_CHANGE << 16):
				cfg_main_new.allowed_protocols = trim(uGetDlgItemText(*this, nID));
				break;
			case IDC_STOP_AFTER_QUEUE_ENABLE | (BN_CLICKED << 16):
				cfg_main_new.stop_after_queue_enable = IsDlgButtonChecked(nID) == BST_CHECKED;
				break;
			case IDC_GZIP_ENABLE | (BN_CLICKED << 16):
				cfg_main_new.gzip_enable = IsDlgButtonChecked(nID) == BST_CHECKED;
				break;
		}

		onChanged();
	}

	void onChanged() {
		m_callback->on_state_changed();
	}

	const preferences_page_callback::ptr m_callback;
};

class preferences_page_main_impl : public preferences_page_impl<preferences_page_main> {
public:
	const char* get_name() { return "HTTP Control"; }
	GUID get_guid() { return guid_preferences_page1; }
	GUID get_parent_guid() { return preferences_page::guid_tools; }
	bool get_help_url(pfc::string_base& p_out) { p_out = VER_SUPPORT_FORUM; return true; }
};

static preferences_page_factory_t<preferences_page_main_impl> g_preferences_page1_impl_factory;


class preferences_page_query : public preferences_page_instance, public CDialogImpl<preferences_page_query>
{
private:
	fb2k::CCoreDarkModeHooks m_hooks;

	config_query cfg_query_new;

	void updateDialog() {
		uSetDlgItemText(m_hWnd, IDC_AUTOPLAYLIST_SORT_PATTERN, cfg_query_new.sortpattern);
		uSetDlgItemText(m_hWnd, IDC_AUTOPLAYLIST_QUERY_STEP1, cfg_query_new.step1);
		uSetDlgItemText(m_hWnd, IDC_AUTOPLAYLIST_QUERY_STEP2, cfg_query_new.step2);
		uSetDlgItemText(m_hWnd, IDC_AUTOPLAYLIST_QUERY_STEP3, cfg_query_new.step3);
		CheckDlgButton(IDC_AUTOPLAYLIST_SENDTODEDICATED, cfg_query_new.sendtodedicated);
	}

public:
	enum { IDD = IDD_TAB3 };

	preferences_page_query(preferences_page_callback::ptr callback) : m_callback(callback) { cfg_query_new = cfg.query; }

	t_uint32 get_state() {
		t_uint32 state = preferences_state::resettable;
		if (!(cfg.query == cfg_query_new)) state |= preferences_state::changed;

		return state | preferences_state::dark_mode_supported;
	}

	void apply() {
		cfg.query = cfg_query_new;
		onChanged();
	}

	void reset() {
		cfg_query_new.reset();
		updateDialog();
		onChanged();
	}

	HWND get_wnd() { return m_hWnd; }

	BEGIN_MSG_MAP_EX(preferences_page_query)
		MSG_WM_INITDIALOG(OnInitDialog)
		MSG_WM_COMMAND(OnCommand)
		END_MSG_MAP()

	BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam) {
		m_hooks.AddDialogWithControls(*this);

		updateDialog();
		return TRUE;
	}

	void OnCommand(UINT uNotifyCode, int nID, CWindow wndCtl) {
		switch (nID | (uNotifyCode << 16))
		{
		case (EN_CHANGE << 16) | IDC_AUTOPLAYLIST_SORT_PATTERN:
			cfg_query_new.sortpattern = trim(uGetDlgItemText(*this, nID));
			break;
		case (EN_CHANGE << 16) | IDC_AUTOPLAYLIST_QUERY_STEP1:
			cfg_query_new.step1 = trim(uGetDlgItemText(*this, nID));
			break;
		case (EN_CHANGE << 16) | IDC_AUTOPLAYLIST_QUERY_STEP2:
			cfg_query_new.step2 = trim(uGetDlgItemText(*this, nID));
			break;
		case (EN_CHANGE << 16) | IDC_AUTOPLAYLIST_QUERY_STEP3:
			cfg_query_new.step3 = trim(uGetDlgItemText(*this, nID));
			break;
		case IDC_AUTOPLAYLIST_SENDTODEDICATED:
			cfg_query_new.sendtodedicated = IsDlgButtonChecked(nID) == BST_CHECKED;
			break;
		}

		onChanged();
	}

	void onChanged() {
		m_callback->on_state_changed();
	}

	const preferences_page_callback::ptr m_callback;
};

class preferences_page_query_impl : public preferences_page_impl<preferences_page_query> {
public:
	const char* get_name() { return "Media Library"; }
	GUID get_guid() { return guid_preferences_page2; }
	GUID get_parent_guid() { return guid_preferences_page1; }
	bool get_help_url(pfc::string_base& p_out) { p_out = VER_SUPPORT_FORUM; return true; }
};

static preferences_page_factory_t<preferences_page_query_impl> g_preferences_page1_query_factory;
} //namespace