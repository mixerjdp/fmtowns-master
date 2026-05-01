// license:BSD-3-Clause
// copyright-holders:OpenAI

#include "mame.h"

const char *emulator_info::get_bare_build_version()
{
	return bare_build_version;
}

const char *emulator_info::get_build_version()
{
	return build_version;
}

void emulator_info::display_ui_chooser(running_machine &machine)
{
	(void)machine;
}

int emulator_info::start_frontend(emu_options &options, osd_interface &osd, std::vector<std::string> &args)
{
	(void)options;
	(void)osd;
	(void)args;
	return EMU_ERR_NONE;
}

int emulator_info::start_frontend(emu_options &options, osd_interface &osd, int argc, char *argv[])
{
	std::vector<std::string> args;
	args.reserve(argc > 0 ? argc : 0);
	for (int i = 0; i < argc; ++i)
		args.emplace_back(argv[i] != nullptr ? argv[i] : "");
	return start_frontend(options, osd, args);
}

bool emulator_info::draw_user_interface(running_machine &machine)
{
	(void)machine;
	return false;
}

void emulator_info::periodic_check()
{
}

bool emulator_info::frame_hook()
{
	return false;
}

void emulator_info::sound_hook(const std::map<std::string, std::vector<std::pair<const float *, int>>> &sound)
{
	(void)sound;
}

void emulator_info::layout_script_cb(layout_file &file, const char *script)
{
	(void)file;
	(void)script;
}

bool emulator_info::standalone()
{
	return false;
}
