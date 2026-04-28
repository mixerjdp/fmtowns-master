#ifndef FMTOWNS_MAME_BRIDGE_H
#define FMTOWNS_MAME_BRIDGE_H

#include <cstdint>
#include <cstddef>
#include <memory>
#include <string>

namespace fmtowns::mame_bridge {

struct boot_config
{
	std::string model;
	std::string bios_directory;
	std::string content_path;
	std::string cfg_directory;
	std::string nvram_directory;
	std::string ram_size;
	std::string pad1_device;
	std::string pad2_device;
};

struct runtime_snapshot
{
	double machine_time_seconds = 0.0;
	uint64_t screen_frame = 0;
	uint64_t cpu_pc = 0;
	unsigned phase = 0;
};

class session
{
public:
	session();
	~session();

	session(const session &) = delete;
	session &operator=(const session &) = delete;

	bool start(const boot_config &config, std::string &error);
	void run_slice();
	void log_boot_snapshot(const char *stage);
	void log_video_snapshot(const char *stage);
	void force_video_update();
	bool execution_snapshot(runtime_snapshot &snapshot) const;
	size_t savestate_size(std::string &error) const;
	bool save_state(void *data, size_t size, std::string &error);
	bool load_state(const void *data, size_t size, std::string &error);
	void advance_savestate_guard();
	void reset();
	void stop();
	bool running() const;
	bool copy_video_frame(uint32_t *pixels, unsigned max_width, unsigned max_height, unsigned &width, unsigned &height);
	void set_joystick_input(unsigned player, bool up, bool down, bool left, bool right, bool button1, bool button2, bool start, bool select);

private:
	class impl;
	std::unique_ptr<impl> m_impl;
};

} // namespace fmtowns::mame_bridge

#endif
