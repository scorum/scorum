#pragma once

#include "parsers.hpp"

#include <boost/filesystem.hpp>

namespace scorum {
namespace util {

class file_parser : public parser_i
{
public:
    explicit file_parser(const boost::filesystem::path&);

    virtual void update(genesis_state_type&) override;

private:
    boost::filesystem::path _path;
};
}
}
