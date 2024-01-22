#pragma once

struct parent_path_info
{
  std::string filename;
  std::string command;
};

struct root_path_info
{
  std::string root_command;
  std::filesystem::path root_command_path;
  std::filesystem::path index_path;
};
