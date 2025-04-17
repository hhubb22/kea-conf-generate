#include "KeaGenerator.h"

namespace KeaGenerator
{
// Converts InterfacesConfig to JSON format.
// Expected JSON: { "interfaces": ["if1", "if2", ...] }
void
to_json (nlohmann::json &j, const InterfacesConfig &i)
{
    // Assign a JSON object with the "interfaces" key holding the
    // vector.
    j = nlohmann::json{ { "interfaces", i.interfaces } };
}

// Converts LeaseDatabase to JSON format.
// Expected JSON: { "type": "...", "persist": ..., "name": "..." }
void
to_json (nlohmann::json &j, const LeaseDatabase &l)
{
    // Assign individual key-value pairs to the JSON object.
    j["type"] = l.type;
    j["persist"] = l.persist;
    j["name"] = l.name;
}

// Converts OptionData::Option to JSON format.
// Expected JSON: { "name": "...", "data": "...", "always-send": ... }
void
to_json (nlohmann::json &j, const OptionData::Option &o)
{
    j = nlohmann::json{ { "name", o.name },
                        { "data", o.data },
                        { "always-send", o.always_send } };
}

// Converts the entire OptionData structure to JSON format.
// Expected JSON: [ { Option1 }, { Option2 }, ... ] (Array of Option
// objects)
void
to_json (nlohmann::json &j, const OptionData &o)
{
    // nlohmann/json automatically handles serialization of
    // std::set<Option>.
    j = o.options;
}

// Converts Subnet4::Pool to JSON format.
// Expected JSON: { "pool": "low_ip - high_ip" }
void
to_json (nlohmann::json &j, const Subnet4::Pool &p)
{
    j = nlohmann::json{ { "pool", p.range } };
}

// Converts Subnet4::Cfg to JSON format.
// Expected JSON: { "id": ..., "subnet": "...", "pools": [ { "pool":
// "..." }, ... ] }
void
to_json (nlohmann::json &j, const Subnet4::Cfg &c)
{
    j = nlohmann::json{
        { "id", c.id }, { "subnet", c.subnet }, { "pools", c.pools }
    }; // nlohmann/json handles set<Pool> serialization
}

// Converts the entire Subnet4 structure to JSON format.
// Expected JSON: [ { Cfg1 }, { Cfg2 }, ... ] (Array of Cfg objects)
void
to_json (nlohmann::json &j, const Subnet4 &s)
{
    // Create a temporary vector to hold Cfg objects for
    // serialization. The order in the final JSON array might not be
    // guaranteed due to unordered_map iteration. If consistent order
    // is needed, consider sorting by ID here or using std::map.
    std::vector<Subnet4::Cfg> cfg_vector;
    cfg_vector.reserve (
        s.cfgs.size ()); // Reserve space for efficiency
    for (const auto &pair : s.cfgs)
    {
        cfg_vector.push_back (pair.second);
    }

    // Serialize the vector of configurations.
    j = cfg_vector;
}

// Converts Dhcp4 structure to JSON format.
// Includes nested structures if they are not empty.
void
to_json (nlohmann::json &j, const Dhcp4 &d)
{
    // Start with the mandatory valid-lifetime field.
    j = nlohmann::json{ { "valid-lifetime", d.valid_lifetime } };

    // --- Include optional/required sections only if they are
    // valid/non-empty --- Kea typically requires interfaces, lease
    // database, and at least one subnet. Option-data is often
    // required in practice too.

    if (d.interface_config.empty ())
    {
        // Depending on requirements, either throw, log, or omit the
        // field. Omitting might lead to an invalid Kea config.
        // Current behaviour matches original: prints error, returns
        // partial JSON. Consider throwing std::runtime_error here for
        // stricter validation.
        std::cerr
            << "interfaces-config is empty during JSON serialization"
            << std::endl;
        // Or: throw std::runtime_error("Cannot serialize Dhcp4:
        // interfaces-config is empty");
        return; // Stops serialization here in the original code
    }
    j["interfaces-config"]
        = d.interface_config; // Serialize interfaces

    if (d.lease_database.empty ())
    {
        std::cerr
            << "lease database is empty during JSON serialization"
            << std::endl;
        // Or: throw std::runtime_error("Cannot serialize Dhcp4:
        // lease-database is empty");
        return; // Stops serialization here
    }
    j["lease-database"]
        = d.lease_database; // Serialize lease database

    if (d.subnet4.empty ())
    {
        std::cerr << "subnet4 is empty during JSON serialization"
                  << std::endl;
        // Or: throw std::runtime_error("Cannot serialize Dhcp4:
        // subnet4 is empty");
        return; // Stops serialization here
    }
    j["subnet4"] = d.subnet4; // Serialize subnets

    // Option data might be considered optional by Kea itself, but
    // often needed. We only include it if it's not empty.
    if (!d.option_data.empty ())
    {
        j["option-data"]
            = d.option_data; // Serialize options if present
    }
    // Note: The original code had an early return if option_data was
    // empty. Changed to only add the key if it's *not* empty,
    // allowing serialization to complete even without options. Adjust
    // if strict requirement is needed.
}

// Converts KeaConfig to the final JSON output format.
// Expected JSON: { "Dhcp4": { ... Dhcp4 JSON ... } }
void
to_json (nlohmann::json &j, const KeaConfig &k)
{
    // Create the top-level object with the "Dhcp4" key.
    j = nlohmann::json{ { "Dhcp4", k.dhcp4 } };
}
}