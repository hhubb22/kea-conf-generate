#include "KeaGenerator.h"
#include <gtest/gtest.h>
#include <set>
#include <string>
#include <vector>

// Use namespaces for convenience
using namespace KeaGenerator;
using json = nlohmann::json;

// --- Test Fixture (Optional but can be useful for shared setup) ---
class KeaGeneratorTest : public ::testing::Test
{
  protected:
    // Per-test setup can go here if needed
    void
    SetUp () override
    {
        // Example: Reset shared resources or variables before each
        // test
    }

    // Per-test tear-down can go here if needed
    void
    TearDown () override
    {
        // Example: Clean up resources
    }

    // Helper function to compare JSON objects robustly
    // Ignores insignificant differences like key order in objects.
    void
    AssertJsonEq (const json &actual, const json &expected)
    {
        ASSERT_EQ (actual, expected)
            << "Actual JSON: " << actual.dump (2)
            << "\nExpected JSON: " << expected.dump (2);
    }
};

// --- InterfacesConfig Tests ---

// Test the constructor and the empty() method
TEST_F (KeaGeneratorTest, InterfacesConfig_ConstructionAndEmpty)
{
    InterfacesConfig empty_config ({}); // Empty initializer list
    EXPECT_TRUE (empty_config.empty ());
    EXPECT_TRUE (empty_config.interfaces.empty ());

    InterfacesConfig config1 ({ "eth0" });
    EXPECT_FALSE (config1.empty ());
    ASSERT_EQ (config1.interfaces.size (), 1);
    EXPECT_EQ (config1.interfaces[0], "eth0");

    InterfacesConfig config2 ({ "eth0", "eth1" });
    EXPECT_FALSE (config2.empty ());
    ASSERT_EQ (config2.interfaces.size (), 2);
    EXPECT_EQ (config2.interfaces[0], "eth0");
    EXPECT_EQ (config2.interfaces[1], "eth1");
}

// Test the JSON serialization of InterfacesConfig
TEST_F (KeaGeneratorTest, InterfacesConfig_Serialization)
{
    InterfacesConfig config ({ "eth0", "lo" });
    json j = config;

    // clang-format off
    json expected_json = R"(
        {
            "interfaces": ["eth0", "lo"]
        }
    )"_json; // Raw string literal parsed as JSON
    // clang-format on

    AssertJsonEq (j, expected_json);

    // Test empty case
    InterfacesConfig empty_config ({});
    json j_empty = empty_config;
    // clang-format off
    json expected_empty = R"(
        {
            "interfaces": []
        }
     )"_json;
    // clang-format on
    AssertJsonEq (j_empty, expected_empty);
}

// --- LeaseDatabase Tests ---

// Test constructors and the empty() method
TEST_F (KeaGeneratorTest, LeaseDatabase_ConstructionAndEmpty)
{
    LeaseDatabase default_db;
    EXPECT_TRUE (
        default_db
            .empty ()); // Default should be empty (empty type/name)
    EXPECT_EQ (default_db.type, "");
    EXPECT_FALSE (default_db.persist);
    EXPECT_EQ (default_db.name, "");

    LeaseDatabase db1 ("memfile", true, "/path/leases");
    EXPECT_FALSE (db1.empty ());
    EXPECT_EQ (db1.type, "memfile");
    EXPECT_TRUE (db1.persist);
    EXPECT_EQ (db1.name, "/path/leases");

    // Test boundary conditions for empty()
    LeaseDatabase db_no_type ("", true, "/path/leases");
    EXPECT_TRUE (db_no_type.empty ()); // Type is empty

    LeaseDatabase db_no_name ("memfile", false, "");
    EXPECT_TRUE (db_no_name.empty ()); // Name is empty
}

// Test the JSON serialization of LeaseDatabase
TEST_F (KeaGeneratorTest, LeaseDatabase_Serialization)
{
    LeaseDatabase db ("mysql", false, "user=kea pass=kea db=kea");
    json j = db; // Calls to_json

    // clang-format off
    json expected_json = R"(
        {
            "type": "mysql",
            "persist": false,
            "name": "user=kea pass=kea db=kea"
        }
    )"_json;
    // clang-format on

    AssertJsonEq (j, expected_json);

    // Test default serialization (although it's considered 'empty')
    LeaseDatabase default_db;
    json j_default = default_db;
    // clang-format off
    json expected_default = R"(
        {
            "type": "",
            "persist": false,
            "name": ""
        }
     )"_json;
    // clang-format on
    AssertJsonEq (j_default, expected_default);
}

// --- Subnet4 Tests ---

// Test Pool comparison operator (used by std::set)
TEST_F (KeaGeneratorTest, Subnet4_Pool_Comparison)
{
    Subnet4::Pool p1{ "192.168.1.10 - 192.168.1.20" };
    Subnet4::Pool p2{ "192.168.1.50 - 192.168.1.60" };
    Subnet4::Pool p3{ "192.168.1.10 - 192.168.1.20" };
    EXPECT_TRUE (p1 < p2);
    EXPECT_FALSE (p2 < p1);
    EXPECT_FALSE (p1 < p3); // p1 is not less than p3 (they are equal)
    EXPECT_FALSE (p3 < p1); // p3 is not less than p1 (they are equal)
}

// Test Subnet4 construction, adding configs and pools
TEST_F (KeaGeneratorTest, Subnet4_Management)
{
    Subnet4 s4;
    EXPECT_TRUE (s4.empty ());
    EXPECT_EQ (s4.max_id, 1); // Initial ID

    // Add first config
    uint64_t id1 = s4.add_config ("192.168.1.0/24");
    EXPECT_EQ (id1, 1);
    EXPECT_FALSE (s4.empty ());
    EXPECT_EQ (s4.max_id, 2); // ID should increment
    ASSERT_NE (s4.cfgs.find (id1),
               s4.cfgs.end ()); // Check config exists
    EXPECT_EQ (s4.cfgs[id1].subnet, "192.168.1.0/24");
    EXPECT_TRUE (s4.cfgs[id1].pools.empty ());

    // Add second config
    uint64_t id2 = s4.add_config ("10.0.0.0/8");
    EXPECT_EQ (id2, 2);
    EXPECT_EQ (s4.max_id, 3);
    ASSERT_NE (s4.cfgs.find (id2), s4.cfgs.end ());
    EXPECT_EQ (s4.cfgs[id2].subnet, "10.0.0.0/8");

    // Add pool to first config
    bool added1
        = s4.add_pool_for_cfg (id1, "192.168.1.100", "192.168.1.200");
    EXPECT_TRUE (added1);
    ASSERT_EQ (s4.cfgs[id1].pools.size (), 1);
    EXPECT_EQ (s4.cfgs[id1].pools.begin ()->range,
               "192.168.1.100 - 192.168.1.200");

    // Add another pool to first config (should be ordered)
    bool added2
        = s4.add_pool_for_cfg (id1, "192.168.1.50", "192.168.1.60");
    EXPECT_TRUE (added2);
    ASSERT_EQ (s4.cfgs[id1].pools.size (), 2);
    // Verify order due to std::set
    auto it = s4.cfgs[id1].pools.begin ();
    EXPECT_EQ (it->range,
               "192.168.1.100 - 192.168.1.200"); // String comparison
                                                 // puts this first
    ++it;
    EXPECT_EQ (it->range, "192.168.1.50 - 192.168.1.60");

    // Try adding pool to non-existent config
    bool added_invalid
        = s4.add_pool_for_cfg (999, "1.1.1.1", "1.1.1.1");
    EXPECT_FALSE (added_invalid);
}

// Test JSON serialization of Subnet4 (including nested Cfg and Pool)
TEST_F (KeaGeneratorTest, Subnet4_Serialization)
{
    Subnet4 s4;
    uint64_t id1 = s4.add_config ("192.168.1.0/24");
    s4.add_pool_for_cfg (id1, "192.168.1.100", "192.168.1.200");
    s4.add_pool_for_cfg (id1, "192.168.1.50",
                         "192.168.1.60"); // Add a second pool

    uint64_t id2 = s4.add_config ("10.0.0.0/8");
    s4.add_pool_for_cfg (id2, "10.1.2.3",
                         "10.1.2.3"); // Single address pool

    json j = s4; // Calls to_json

    // Expected JSON - Note: Array order depends on unordered_map
    // iteration, so we parse and compare JSON objects which handles
    // order differences. Pool order within a Cfg IS deterministic due
    // to std::set.

    // clang-format off
    json expected_json = R"(
        [
            {
                "id": 1,
                "subnet": "192.168.1.0/24",
                "pools": [
                    { "pool": "192.168.1.100 - 192.168.1.200" },
                    { "pool": "192.168.1.50 - 192.168.1.60" }
                ]
            },
            {
                "id": 2,
                "subnet": "10.0.0.0/8",
                "pools": [
                     { "pool": "10.1.2.3 - 10.1.2.3" }
                ]
            }
        ]
    )"_json;
    // clang-format on

    // We need to compare contents without relying on array order from
    // unordered_map
    ASSERT_EQ (j.size (), expected_json.size ());
    // Convert actual JSON array to a set of JSON objects for
    // comparison
    std::set<json> actual_set (j.begin (), j.end ());
    std::set<json> expected_set (expected_json.begin (),
                                 expected_json.end ());
    ASSERT_EQ (actual_set, expected_set)
        << "Actual JSON: " << j.dump (2)
        << "\nExpected JSON: " << expected_json.dump (2);

    // Test empty Subnet4 serialization
    Subnet4 empty_s4;
    json j_empty = empty_s4;
    // clang-format off
    json expected_empty = R"([])"_json;
    // clang-format on
    AssertJsonEq (j_empty, expected_empty);
}

// --- OptionData Tests ---

// Test Option comparison operator (used by std::set)
TEST_F (KeaGeneratorTest, OptionData_Option_Comparison)
{
    OptionData::Option o1{ "domain-name-servers", "8.8.8.8", true };
    OptionData::Option o2{ "routers", "192.168.1.1", false };
    OptionData::Option o3{ "domain-name-servers", "1.1.1.1",
                           false }; // Same name as o1

    EXPECT_TRUE (o1 < o2); // 'd' < 'r'
    EXPECT_FALSE (o2 < o1);
    EXPECT_FALSE (o1 < o3); // Not less, name is equal
    EXPECT_FALSE (o3 < o1); // Not less, name is equal
}

// Test OptionData construction and adding options
TEST_F (KeaGeneratorTest, OptionData_Management)
{
    OptionData od;
    EXPECT_TRUE (od.empty ());

    // Add regular option
    od.add_option ("routers", "192.168.1.1", false);
    EXPECT_FALSE (od.empty ());
    ASSERT_EQ (od.options.size (), 1);
    const auto &opt1 = *od.options.begin ();
    EXPECT_EQ (opt1.name, "routers");
    EXPECT_EQ (opt1.data, "192.168.1.1");
    EXPECT_FALSE (opt1.always_send);

    // Add always-send option
    od.add_option_always ("domain-name-servers", "8.8.8.8, 1.1.1.1");
    ASSERT_EQ (od.options.size (), 2);
    // Find the added option (set order is deterministic)
    auto it = od.options.find (
        { "domain-name-servers", "", false }); // Search by name
    ASSERT_NE (it, od.options.end ());
    EXPECT_EQ (it->name, "domain-name-servers");
    EXPECT_EQ (it->data, "8.8.8.8, 1.1.1.1");
    EXPECT_TRUE (it->always_send);

    // Try adding option with the same name (should not replace due to
    // set)
    od.add_option ("routers", "192.168.2.1", true);
    ASSERT_EQ (od.options.size (), 2); // Size should remain 2
    // Verify the original router option is still there
    auto it_router = od.options.find ({ "routers", "", false });
    ASSERT_NE (it_router, od.options.end ());
    EXPECT_EQ (it_router->data, "192.168.1.1"); // Original data
    EXPECT_FALSE (it_router->always_send); // Original always_send
}

// Test JSON serialization of OptionData
TEST_F (KeaGeneratorTest, OptionData_Serialization)
{
    OptionData od;
    od.add_option ("routers", "192.168.1.1", false);
    od.add_option_always ("domain-name-servers", "8.8.8.8, 1.1.1.1");
    od.add_option ("domain-name", "example.com", true);

    json j = od; // Calls to_json

    // Expected JSON - Array order IS deterministic because std::set
    // orders by name

    // clang-format off
    json expected_json = R"(
        [
            {
                "name": "domain-name",
                "data": "example.com",
                "always-send": true
            },
            {
                "name": "domain-name-servers",
                "data": "8.8.8.8, 1.1.1.1",
                "always-send": true
            },
            {
                "name": "routers",
                "data": "192.168.1.1",
                "always-send": false
            }
        ]
    )"_json;
    // clang-format on

    AssertJsonEq (j, expected_json);

    // Test empty serialization
    OptionData empty_od;
    json j_empty = empty_od;

    // clang-format off
    json expected_empty = R"([])"_json;
    // clang-format on

    AssertJsonEq (j_empty, expected_empty);
}

// --- Dhcp4 Tests ---

// Test Dhcp4 constructor
TEST_F (KeaGeneratorTest, Dhcp4_Construction)
{
    Dhcp4 dhcp4 (3600, { "eth0", "eth1" }, "memfile", true,
                 "/tmp/leases.db");

    EXPECT_EQ (dhcp4.valid_lifetime, 3600);

    ASSERT_FALSE (dhcp4.interface_config.empty ());
    ASSERT_EQ (dhcp4.interface_config.interfaces.size (), 2);
    EXPECT_EQ (dhcp4.interface_config.interfaces[0], "eth0");
    EXPECT_EQ (dhcp4.interface_config.interfaces[1], "eth1");

    ASSERT_FALSE (dhcp4.lease_database.empty ());
    EXPECT_EQ (dhcp4.lease_database.type, "memfile");
    EXPECT_TRUE (dhcp4.lease_database.persist);
    EXPECT_EQ (dhcp4.lease_database.name, "/tmp/leases.db");

    EXPECT_TRUE (
        dhcp4.subnet4.empty ()); // Subnet should be empty initially
    EXPECT_TRUE (dhcp4.option_data
                     .empty ()); // Options should be empty initially
}

// Test Dhcp4 JSON serialization (Happy Path - all required parts
// present)
TEST_F (KeaGeneratorTest, Dhcp4_Serialization_HappyPath)
{
    // Create a fully configured Dhcp4 object
    Dhcp4 dhcp4 (7200, { "enp0s1" }, "memfile", true,
                 "kea-leases4.csv");

    // Add subnet config
    uint64_t subnet_id = dhcp4.subnet4.add_config ("192.168.50.0/24");
    dhcp4.subnet4.add_pool_for_cfg (subnet_id, "192.168.50.10",
                                    "192.168.50.20");

    // Add option data
    dhcp4.option_data.add_option_always ("domain-name-servers",
                                         "192.168.50.1, 8.8.8.8");
    dhcp4.option_data.add_option ("routers", "192.168.50.1", false);

    json j = dhcp4; // Calls to_json

    // Define the expected JSON structure

    // clang-format off
    json expected_json = R"(
        {
            "valid-lifetime": 7200,
            "interfaces-config": {
                "interfaces": ["enp0s1"]
            },
            "lease-database": {
                "type": "memfile",
                "persist": true,
                "name": "kea-leases4.csv"
            },
            "subnet4": [
                {
                    "id": 1,
                    "subnet": "192.168.50.0/24",
                    "pools": [
                        { "pool": "192.168.50.10 - 192.168.50.20" }
                    ]
                }
            ],
            "option-data": [
                 {
                    "name": "domain-name-servers",
                    "data": "192.168.50.1, 8.8.8.8",
                    "always-send": true
                },
                {
                    "name": "routers",
                    "data": "192.168.50.1",
                    "always-send": false
                }
            ]
        }
    )"_json;
    // clang-format on

    AssertJsonEq (j, expected_json);
}

// Test Dhcp4 JSON serialization when optional OptionData is missing
TEST_F (KeaGeneratorTest, Dhcp4_Serialization_NoOptions)
{
    // Create Dhcp4 without adding any options
    Dhcp4 dhcp4 (3000, { "ethX" }, "memfile", false, "leases.db");
    uint64_t subnet_id = dhcp4.subnet4.add_config ("10.0.1.0/24");
    dhcp4.subnet4.add_pool_for_cfg (subnet_id, "10.0.1.100",
                                    "10.0.1.150");

    json j = dhcp4;

    // Expected JSON should NOT contain the "option-data" key

    // clang-format off
    json expected_json = R"(
        {
            "valid-lifetime": 3000,
            "interfaces-config": {
                "interfaces": ["ethX"]
            },
            "lease-database": {
                "type": "memfile",
                "persist": false,
                "name": "leases.db"
            },
            "subnet4": [
                {
                    "id": 1,
                    "subnet": "10.0.1.0/24",
                    "pools": [
                        { "pool": "10.0.1.100 - 10.0.1.150" }
                    ]
                }
            ]
        }
    )"_json;
    // clang-format on

    AssertJsonEq (j, expected_json);
}

// NOTE: Testing the error cases (empty interfaces/lease/subnet) in
// Dhcp4::to_json is tricky because the original code prints to stderr
// and returns partially formed JSON. A robust test would require
// capturing stderr or refactoring the error handling (e.g., throwing
// exceptions). These tests focus on valid configuration generation.

// --- KeaConfig Tests ---

// Test KeaConfig construction (default and parameterized)
TEST_F (KeaGeneratorTest, KeaConfig_Construction)
{
    // Test default constructor
    KeaConfig default_config;
    EXPECT_EQ (default_config.dhcp4.valid_lifetime, 4000);
    ASSERT_EQ (
        default_config.dhcp4.interface_config.interfaces.size (), 2);
    EXPECT_EQ (default_config.dhcp4.interface_config.interfaces[0],
               "aaa");
    EXPECT_EQ (default_config.dhcp4.interface_config.interfaces[1],
               "bbb");
    EXPECT_EQ (default_config.dhcp4.lease_database.type,
               "memfile"); // Default from Dhcp4 ctor

    // Test parameterized constructor
    Dhcp4 custom_dhcp4 (1000, { "lo" });
    KeaConfig param_config (custom_dhcp4);
    EXPECT_EQ (param_config.dhcp4.valid_lifetime, 1000);
    ASSERT_EQ (param_config.dhcp4.interface_config.interfaces.size (),
               1);
    EXPECT_EQ (param_config.dhcp4.interface_config.interfaces[0],
               "lo");
}

// Test KeaConfig JSON serialization
TEST_F (KeaGeneratorTest, KeaConfig_Serialization)
{
    // Create a KeaConfig and customize its Dhcp4 part
    KeaConfig config;                    // Starts with default Dhcp4
    config.dhcp4.valid_lifetime = 86400; // Change lifetime
    config.dhcp4.lease_database
        = LeaseDatabase ("mysql", true, "db=kea"); // Change DB
    uint64_t id = config.dhcp4.subnet4.add_config (
        "172.16.0.0/16"); // Add subnet
    config.dhcp4.subnet4.add_pool_for_cfg (id, "172.16.10.1",
                                           "172.16.10.254");
    config.dhcp4.option_data.add_option_always (
        "domain-name-servers", "172.16.0.1"); // Add option

    json j = config; // Calls to_json for KeaConfig

    // Expected top-level structure with nested Dhcp4

    // clang-format off
    json expected_json = R"(
        {
            "Dhcp4": {
                 "valid-lifetime": 86400,
                 "interfaces-config": {
                     "interfaces": ["aaa", "bbb"]
                 },
                 "lease-database": {
                     "type": "mysql",
                     "persist": true,
                     "name": "db=kea"
                 },
                 "subnet4": [
                     {
                         "id": 1,
                         "subnet": "172.16.0.0/16",
                         "pools": [
                             { "pool": "172.16.10.1 - 172.16.10.254" }
                         ]
                     }
                 ],
                 "option-data": [
                     {
                         "name": "domain-name-servers",
                         "data": "172.16.0.1",
                         "always-send": true
                     }
                 ]
            }
        }
    )"_json;
    // clang-format on

    AssertJsonEq (j, expected_json);
}

// --- Main function for GTest ---
int
main (int argc, char **argv)
{
    ::testing::InitGoogleTest (&argc, argv);
    return RUN_ALL_TESTS ();
}
// Usually, you link against the gtest_main library which provides
// main(). If not, uncomment the main function above.