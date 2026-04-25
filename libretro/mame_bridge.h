#ifndef FMTOWNS_MAME_BRIDGE_H
#define FMTOWNS_MAME_BRIDGE_H

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
	void reset();
	void stop();
	bool running() const;
	bool copy_video_frame(uint32_t *pixels, unsigned max_width, unsigned max_height, unsigned &width, unsigned &height);

private:
	class impl;
	std::unique_ptr<impl> m_impl;
};

} // namespace fmtowns::mame_bridge

#endif
