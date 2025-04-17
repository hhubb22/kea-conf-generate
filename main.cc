#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "json.hpp"

namespace KeaGenerator
{
struct InterfacesConfig
{
    InterfacesConfig (std::initializer_list<std::string> init_list)
        : interfaces (init_list)
    {
    }

    bool
    empty () const
    {
        return interfaces.empty ();
    }

    std::vector<std::string> interfaces;
};

void
to_json (nlohmann::json &j, const InterfacesConfig &i)
{
    j = nlohmann::json{ { "interfaces", i.interfaces } };

    return;
}

struct LeaseDatabase
{
    LeaseDatabase (std::string t, bool p, std::string n)
        : type (std::move (t)), persist (p), name (std::move (n))
    {
    }

    LeaseDatabase () : type (""), persist (false), name ("") {}

    bool
    empty () const
    {
        if (type == "" || name == "")
        {
            return true;
        }

        return false;
    }

    std::string type;
    bool persist;
    std::string name;
};

void
to_json (nlohmann::json &j, const LeaseDatabase &l)
{
    j["type"] = l.type;
    j["persist"] = l.persist;
    j["name"] = l.name;
}

struct Subnet4
{
    struct Pool
    {
        bool
        operator< (const Pool &rhs) const
        {
            return range < rhs.range;
        }
        std::string range;
    };
    struct Cfg
    {
        uint64_t id;
        std::string subnet;
        std::set<Pool> pools;
    };

    Subnet4 () : max_id (1) {}

    uint64_t
    add_config (std::string subnet)
    {
        cfgs[max_id] = Cfg{ max_id, std::move (subnet), {} };
        return max_id++;
    }

    bool
    add_pool_for_cfg (uint64_t cfg_id, std::string low,
                      std::string high)
    {
        if (cfgs.find (cfg_id) == cfgs.end ())
        {
            return false;
        }

        std::string pool = std::move (low) + " - " + std::move (high);
        cfgs[cfg_id].pools.insert ({ std::move (pool) });
        return true;
    }

    bool
    empty () const
    {
        return cfgs.empty ();
    }

    uint64_t max_id;
    std::unordered_map<uint64_t, Cfg> cfgs;
};

void
to_json (nlohmann::json &j, const Subnet4::Pool &p)
{
    j = nlohmann::json{ { "pool", p.range } };
}

void
to_json (nlohmann::json &j, const Subnet4::Cfg &c)
{
    j = nlohmann::json{ { "id", c.id },
                        { "subnet", c.subnet },
                        { "pools", c.pools } };
}

void
to_json (nlohmann::json &j, const Subnet4 &s)
{
    std::vector<Subnet4::Cfg> cfgs;
    for (auto &cfg : s.cfgs)
    {
        cfgs.push_back (cfg.second);
    }

    j = cfgs;
}

struct OptionData
{
    struct Option
    {
        std::string name;
        std::string data;

        bool always_send;

        bool
        operator< (const Option &rhs) const
        {
            return name < rhs.name;
        }
    };

    OptionData () {}

    void
    add_option_always (std::string name, std::string data)
    {
        add_option (name, data, true);
        return;
    }

    void
    add_option (std::string name, std::string data, bool always_send)
    {
        options.insert (Option{ std::move (name), std::move (data),
                                always_send });
        return;
    }

    bool
    empty () const
    {
        return options.empty ();
    }

    std::set<Option> options;
};

void
to_json (nlohmann::json &j, const OptionData::Option &o)
{
    j = nlohmann::json{ { "name", o.name },
                        { "data", o.data },
                        { "always-send", o.always_send } };
}

void
to_json (nlohmann::json &j, const OptionData &o)
{
    j = o.options;
}

struct Dhcp4
{
    Dhcp4 (const uint64_t &lifetime,
           std::initializer_list<std::string> interfaces_init_list,
           std::string lease_type = "memfile",
           bool lease_persist = true,
           std::string lease_name = "/var/lib/kea/dhcp4.leases")
        : valid_lifetime (lifetime),
          interface_config (interfaces_init_list),
          lease_database (lease_type, lease_persist, lease_name)
    {
    }

    uint64_t valid_lifetime;
    InterfacesConfig interface_config;
    LeaseDatabase lease_database;
    Subnet4 subnet4;
    OptionData option_data;
};

void
to_json (nlohmann::json &j, const Dhcp4 &d)
{
    j = nlohmann::json{ { "valid-lifetime", d.valid_lifetime } };
    if (d.interface_config.empty ())
    {
        std::cerr << "interfaces-config is empty" << std::endl;
        return;
    }
    j["interfaces-config"] = d.interface_config;

    if (d.lease_database.empty ())
    {
        std::cerr << "lease database is empty" << std::endl;
        return;
    }
    j["lease-database"] = d.lease_database;

    if (d.subnet4.empty ())
    {
        std::cerr << "subnet4 is empty" << std::endl;
        return;
    }
    j["subnet4"] = d.subnet4;

    if (d.option_data.empty ())
    {
        std::cerr << "option data is empty" << std::endl;
        return;
    }
    j["option-data"] = d.option_data;
}

struct KeaConfig
{
    KeaConfig () : dhcp4 (4000, { "aaa", "bbb" }) {}
    Dhcp4 dhcp4;
};

void
to_json (nlohmann::json &j, const KeaConfig &k)
{
    j = nlohmann::json{ { "Dhcp4", k.dhcp4 } };
}
}

int
main ()
{
    using namespace KeaGenerator;

    KeaConfig config;
    config.dhcp4.subnet4.add_config ("192.168.10.0/24");
    config.dhcp4.subnet4.add_pool_for_cfg (
        config.dhcp4.subnet4.max_id - 1, "192.168.10.10",
        "192.168.10.20");
    config.dhcp4.option_data.add_option_always (
        "domain-name-servers", "192.0.2.1, 192.0.2.2");
    nlohmann::json j = config;
    std::cout << j.dump (2) << std::endl;
    return 0;
}
