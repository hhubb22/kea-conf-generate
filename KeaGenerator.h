// File: KeaGenerator.h
#ifndef KEA_GENERATOR_H
#define KEA_GENERATOR_H

#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <nlohmann/json.hpp>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace KeaGenerator
{
// --- InterfacesConfig ---
// Represents the network interfaces Kea should listen on.
struct InterfacesConfig
{
    // Constructor taking an initializer list of interface names.
    InterfacesConfig (std::initializer_list<std::string> init_list)
        : interfaces (init_list)
    {
    }

    // Default constructor (useful for placeholder/optional scenarios)
    InterfacesConfig () = default;

    // Checks if the list of interfaces is empty.
    // Returns true if no interfaces are configured, false otherwise.
    bool
    empty () const
    {
        return interfaces.empty ();
    }

    // The list of interface names (e.g., "eth0", "ens192").
    std::vector<std::string> interfaces;

    // JSON serialization support
    friend void to_json (nlohmann::json &, const InterfacesConfig &);
};

// --- LeaseDatabase ---
// Configuration for the DHCP lease database.
struct LeaseDatabase
{
    // Parameterized constructor.
    LeaseDatabase (std::string t, bool p, std::string n)
        : type (std::move (t)), persist (p), name (std::move (n))
    {
    }

    // Default constructor initializing members to default/empty
    // states.
    LeaseDatabase () : type (""), persist (false), name ("") {}

    // Checks if the configuration is considered empty or invalid.
    // Returns true if type or name is empty, false otherwise.
    bool
    empty () const
    {
        // Both type and name must be non-empty for a valid
        // configuration.
        return type.empty () || name.empty ();
    }

    // JSON serialization support
    friend void to_json (nlohmann::json &, const LeaseDatabase &);
    operator nlohmann::json () const
    {
        nlohmann::json j;
        to_json (j, *this);
        return j;
    }

    std::string type; // Type of database (e.g., "memfile", "mysql").
    bool persist;     // Should leases persist across restarts?
    std::string name; // Name/path/connection string for the database.
};

// --- Subnet4 ---
// Manages IPv4 subnet configurations, including address pools.
struct Subnet4
{
    // Represents a range of IP addresses available for lease within a
    // subnet.
    struct Pool
    {
        // Comparison operator needed for storing Pools in a std::set.
        // Orders pools based on their range string.
        bool
        operator< (const Pool &rhs) const
        {
            return range < rhs.range;
        }

        // The pool range string (e.g., "192.168.1.100 -
        // 192.168.1.200").
        std::string range;

        // JSON serialization support
        friend void to_json (nlohmann::json &, const Pool &);
    };

    // Represents the configuration for a single IPv4 subnet.
    struct Cfg
    {
        uint64_t
            id; // Unique identifier for the subnet configuration.
        std::string subnet; // Subnet address and mask (e.g.,
                            // "192.168.1.0/24").
        std::set<Pool>
            pools; // Set of address pools within this subnet.

        // JSON serialization support
        friend void to_json (nlohmann::json &, const Cfg &);
        operator nlohmann::json () const
        {
            nlohmann::json j;
            to_json (j, *this);
            return j;
        }
    };

    // Default constructor, initializes the next available ID to 1.
    Subnet4 () : max_id (1) {}

    // Adds a new subnet configuration.
    // Takes the subnet string (e.g., "192.168.1.0/24") as input.
    // Returns the unique ID assigned to the new configuration.
    uint64_t
    add_config (std::string subnet)
    {
        uint64_t current_id
            = max_id++; // Get current ID and increment for next use
        // Create and insert the new configuration into the map.
        cfgs[current_id] = Cfg{ current_id, std::move (subnet), {} };
        return current_id; // Return the ID of the newly added config
    }

    // Adds an address pool to an existing subnet configuration.
    // Takes the target configuration ID, low IP, and high IP of the
    // range. Returns true if the pool was added successfully, false
    // if the cfg_id was not found.
    bool
    add_pool_for_cfg (uint64_t cfg_id, std::string low,
                      std::string high)
    {
        // Find the configuration with the given ID.
        auto it = cfgs.find (cfg_id);
        if (it == cfgs.end ())
        {
            // Configuration ID not found.
            return false;
        }

        // Construct the pool range string.
        std::string pool_range
            = std::move (low) + " - " + std::move (high);
        // Insert the new pool into the set for the found
        // configuration.
        it->second.pools.insert ({ std::move (pool_range) });
        return true;
    }

    // Checks if there are any subnet configurations defined.
    // Returns true if no configurations exist, false otherwise.
    bool
    empty () const
    {
        return cfgs.empty ();
    }

    // JSON serialization support
    friend void to_json (nlohmann::json &, const Subnet4 &);

    // Counter to generate unique IDs for subnet configurations.
    // Starts at 1.
    uint64_t max_id;
    // Map storing subnet configurations, keyed by their unique ID.
    std::unordered_map<uint64_t, Cfg> cfgs;
};

// --- OptionData ---
// Manages DHCP options to be sent to clients.
struct OptionData
{
    // Represents a single DHCP option.
    struct Option
    {
        std::string
            name; // Name of the option (e.g., "domain-name-servers").
        std::string
            data; // Value of the option (e.g., "8.8.8.8, 1.1.1.1").
        bool always_send; // Should this option always be sent, even
                          // if not requested?

        // Comparison operator needed for storing Options in a
        // std::set. Orders options based on their name.
        bool
        operator< (const Option &rhs) const
        {
            return name < rhs.name;
        }

        // JSON serialization support
        friend void to_json (nlohmann::json &, const Option &);
        operator nlohmann::json () const
        {
            nlohmann::json j;
            to_json (j, *this);
            return j;
        }
    };

    // Default constructor.
    OptionData () = default;

    // Adds an option that should always be sent.
    void
    add_option_always (std::string name, std::string data)
    {
        // Delegates to the main add_option method with always_send =
        // true.
        add_option (std::move (name), std::move (data), true);
    }

    // Adds a DHCP option.
    void
    add_option (std::string name, std::string data, bool always_send)
    {
        // Insert the new option into the set. If an option with the
        // same name already exists, it won't be replaced due to
        // std::set properties.
        options.insert (
            { std::move (name), std::move (data), always_send });
    }

    // Checks if any options have been defined.
    // Returns true if no options exist, false otherwise.
    bool
    empty () const
    {
        return options.empty ();
    }

    // JSON serialization support
    friend void to_json (nlohmann::json &, const OptionData &);

    // Set storing the configured DHCP options, ordered by name.
    std::set<Option> options;
};

// --- Dhcp4 ---
// Top-level structure representing the Kea DHCPv4 service
// configuration.
struct Dhcp4
{

    // Constructor initializing mandatory fields and defaults for
    // others.
    Dhcp4 (const uint64_t &lifetime,
           std::initializer_list<std::string> interfaces_init_list,
           std::string lease_type = "memfile",
           bool lease_persist = true,
           std::string lease_name = "/var/lib/kea/dhcp4.leases")
        : valid_lifetime (lifetime), // Set lease lifetime
          interface_config (
              interfaces_init_list), // Initialize interfaces
          lease_database (
              std::move (lease_type), // Initialize lease database
              lease_persist, std::move (lease_name)),
          subnet4 (),    // Initialize subnets (empty)
          option_data () // Initialize options (empty)
    {
        // Basic validation: Ensure interfaces are provided.
        if (interface_config.empty ())
        {
            // Consider throwing an exception or logging a critical
            // error instead of just printing to cerr in a library
            // context. throw std::runtime_error("InterfacesConfig
            // cannot be empty for Dhcp4");
            std::cerr << "[Warning] Dhcp4 created with empty "
                         "interfaces-config."
                      << std::endl;
        }
    }

    // Default constructor (maybe less useful here as
    // lifetime/interfaces are key)
    Dhcp4 ()
        : valid_lifetime (0), interface_config (), lease_database (),
          subnet4 (), option_data ()
    {
    }

    // JSON serialization support
    friend void to_json (nlohmann::json &, const Dhcp4 &);

    uint64_t valid_lifetime; // Default lease duration in seconds.
    InterfacesConfig
        interface_config;         // Network interface configuration.
    LeaseDatabase lease_database; // Lease database configuration.
    Subnet4 subnet4;              // IPv4 subnet configurations.
    OptionData option_data;       // DHCP options configuration.
};

// --- KeaConfig ---
// Represents the overall Kea configuration file structure, currently
// only holding Dhcp4.
struct KeaConfig
{

    // Constructor: Initializes Dhcp4 with some default values.
    // Default lifetime 4000s, interfaces "aaa", "bbb".
    // Uses default lease DB settings.
    KeaConfig () : dhcp4 (4000, { "aaa", "bbb" }) {}

    // Parameterized constructor to allow customizing Dhcp4 upon
    // creation
    explicit KeaConfig (Dhcp4 dhcp4_config)
        : dhcp4 (std::move (dhcp4_config))
    {
    }

    // JSON serialization support
    friend void to_json (nlohmann::json &, const KeaConfig &);

    Dhcp4 dhcp4; // The DHCPv4 service configuration block.
};

// Function declarations for JSON serialization
void to_json (nlohmann::json &j, const InterfacesConfig &i);
void to_json (nlohmann::json &j, const LeaseDatabase &l);
void to_json (nlohmann::json &j, const OptionData::Option &o);
void to_json (nlohmann::json &j, const OptionData &o);
void to_json (nlohmann::json &j, const Subnet4::Pool &p);
void to_json (nlohmann::json &j, const Subnet4::Cfg &c);
void to_json (nlohmann::json &j, const Subnet4 &s);
void to_json (nlohmann::json &j, const Dhcp4 &d);
void to_json (nlohmann::json &j, const KeaConfig &k);

} // namespace KeaGenerator

#endif // KEA_GENERATOR_H