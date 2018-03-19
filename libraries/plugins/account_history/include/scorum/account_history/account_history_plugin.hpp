/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#pragma once

#include <scorum/app/plugin.hpp>
#include <scorum/chain/database/database.hpp>

#ifndef ACCOUNT_HISTORY_PLUGIN_NAME
#define ACCOUNT_HISTORY_PLUGIN_NAME "account_history"
#endif

namespace scorum {
namespace account_history {
using namespace chain;
using app::application;

namespace detail {
class account_history_plugin_impl;
}

/**
 *  This plugin is designed to track a range of operations by account so that one node
 *  doesn't need to hold the full operation history in memory.
 */
class account_history_plugin : public scorum::app::plugin
{
public:
    account_history_plugin(application* app);
    virtual ~account_history_plugin();

    std::string plugin_name() const override;
    virtual void plugin_set_program_options(boost::program_options::options_description& cli,
                                            boost::program_options::options_description& cfg) override;
    virtual void plugin_initialize(const boost::program_options::variables_map& options) override;
    virtual void plugin_startup() override;

    flat_map<account_name_type, account_name_type> tracked_accounts() const; /// map start_range to end_range

    friend class detail::account_history_plugin_impl;
    std::unique_ptr<detail::account_history_plugin_impl> my;
};
}
} // scorum::account_history
