#include "stdafx.h"

namespace {
static const GUID g_guid_stop_after_queue = { 0xa643b240, 0x70be, 0x4eba, { 0x92, 0xfd, 0xc4, 0x8, 0x82, 0x7b, 0x28, 0xe3 } };

class stop_after_qeueue_menu_command : public mainmenu_commands
{
public:
	// number of commands
	t_uint32 get_command_count() override
	{
		if (cfg.main.stop_after_queue_enable)
			return 1;
		return 0;
	}

	// All commands are identified by a GUID.
	GUID get_command(t_uint32 p_index) override
	{
		if (cfg.main.stop_after_queue_enable)
			if (p_index == 0) return g_guid_stop_after_queue;

		return pfc::guid_null;
	}

	void get_name(t_uint32 p_index, pfc::string_base& p_out) override
	{
		if (cfg.main.stop_after_queue_enable)
			if (p_index == 0) p_out = "Stop after queue";
	}

	bool get_description(t_uint32 p_index, pfc::string_base& p_out) override
	{
		if (cfg.main.stop_after_queue_enable)
			if (p_index == 0)
			{
				p_out = "Stops playback after completing playback queue.";
				return true;
			}
		return false;
	}

	GUID get_parent() override { return mainmenu_groups::playback_etc; }

	void execute(t_uint32 p_index, service_ptr_t<service_base> p_callback) override
	{
		if (p_index == 0)
			cfg.misc.stop_after_queue = !cfg.misc.stop_after_queue;
	}

	// The standard version of this command does not support checked or disabled
	// commands, so we use our own version.
	bool get_display(t_uint32 p_index, pfc::string_base& p_text, t_uint32& p_flags) override
	{
		if (cfg.main.stop_after_queue_enable)
			if (p_index == 0)
			{
				p_flags = 0;
				if (cfg.misc.stop_after_queue)
					p_flags |= flag_checked;

				get_name(p_index, p_text);
				return true;
			}
		return false;
	}
};

static service_factory_single_t<stop_after_qeueue_menu_command> g_stop_after_qeueue_menu_command;
} // namespace