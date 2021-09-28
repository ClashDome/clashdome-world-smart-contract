#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/transaction.hpp>
#include <atomicassets.hpp>
#include <atomicdata.hpp>

using namespace eosio;
using namespace std;

#define EOSIO name("eosio")
#define CONTRACTN name("clashdomewld")

CONTRACT clashdomewld : public contract {

public:

    using contract::contract;

    ACTION createuser(
        name owner
    );
    ACTION unstake(
        name account,
        uint64_t asset_id
    );
    ACTION withdraw(
        name account,
        vector<asset> quantities
    );
    ACTION craft(
        name account, 
        uint32_t template_id
    );
    ACTION claim(
        name account, 
        vector<uint64_t> asset_ids
    );
    ACTION addmaterials(
        name account,
        vector<asset> quantities
    );
    ACTION setconfig(
        uint16_t init_followers, 
        int32_t reward_noise_min, 
        int32_t reward_noise_max,
        uint8_t min_fee,
        uint8_t max_fee,
        uint8_t fee
    );
    ACTION settoolconfig(
        string tool_name,
        string img,
        name schema_name,
        string type,
        string rarity,
        uint8_t level,
        uint32_t template_id,
        uint16_t stamina_consumed,
        uint16_t experience_consumed,
        vector<asset> mints,
        vector<asset> rewards
    );

    [[eosio::on_notify("atomicassets::transfer")]] void receive_asset_transfer(
        name from,
        name to,
        vector <uint64_t> asset_ids,
        string memo
    );

    [[eosio::on_notify("clashdometkn::transfer")]] void receive_token_transfer(
        name from,
        name to,
        asset quantity,
        string memo
    );

private:

    // TABLES 

    // accounts
    TABLE accounts_s {

        name account;
        vector<asset> balances;
        
        uint64_t primary_key() const { return account.value; }
    };

    typedef multi_index<name("accounts"), accounts_s> accounts_t;

    accounts_t accounts = accounts_t(get_self(), get_self().value); 

    // config
    TABLE config_s {

        uint64_t key;    
        uint16_t init_followers;
        int32_t reward_noise_min;
        int32_t reward_noise_max;
        uint8_t min_fee;
        uint8_t max_fee;
        uint64_t last_fee_updated;
        uint8_t fee;

        uint64_t primary_key() const { return key; }
    };

    typedef multi_index<name("config"), config_s> config_t;

    config_t config = config_t(get_self(), get_self().value); 

    // tool config
    TABLE toolconfig_s {

        string tool_name;
        string img;
        name schema_name;
        string type;
        string rarity;
        uint8_t level;
        uint32_t template_id;
        uint16_t stamina_consumed;
        uint16_t experience_consumed;
        vector<asset> mints;
        vector<asset> rewards;

        uint64_t primary_key() const { return template_id; }
    };

    typedef multi_index<name("toolconfig"), toolconfig_s> toolconfig_t;

    toolconfig_t toolconfig = toolconfig_t(get_self(), get_self().value); 

    // tools
    TABLE tools_s {

        uint64_t asset_id;
        name owner;
        string type;
        uint32_t template_id;

        uint64_t primary_key() const { return asset_id; }
    };

    typedef multi_index<name("tools"), tools_s> tools_t;

    tools_t tools = tools_t(get_self(), get_self().value);

    // AUXILIAR FUNCTIONS
    uint64_t finder(vector<asset> assets, symbol symbol); 

    // CONSTANTS

    const string COLLECTION_NAME = "clashdomewld";
    const string SCHEMA_NAME = "tool";

    static constexpr symbol LUDIO_SYMBOL = symbol(symbol_code("LUDIO"), 4);
    static constexpr symbol STAMINA_SYMBOL = symbol(symbol_code("STAMINA"), 4);
    static constexpr symbol XP_SYMBOL = symbol(symbol_code("XP"), 4);
};