#pragma once

namespace scorum {
namespace chain {

struct i_database_witness_schedule
{
    friend class database;

protected:
    explicit i_database_witness_schedule(database& db)
        : _db(db)
    {
    }

    void _reset_virtual_schedule_time();

    void _update_median_witness_props();

    void update_witness_schedule();

private:
    database& _db;
};
}
}
