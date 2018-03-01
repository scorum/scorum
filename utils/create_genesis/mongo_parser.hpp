#pragma once

#include "parsers.hpp"

#include <string>

namespace scorum {
namespace util {

class mongo_parser : public parser_i
{
public:
    explicit mongo_parser(const std::string& connection_uri);

    virtual void update(genesis_state_type&) override;

private:
    void process_document(const std::string&, genesis_state_type&);

    std::string _connection_uri;
};
}
}
