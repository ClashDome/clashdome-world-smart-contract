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

    ACTION unstake(
        name account,
        uint64_t asset_id,
        string type
    );
    ACTION withdraw(
        name account,
        vector<asset> quantities
    );
    ACTION craft(
        name account, 
        uint32_t template_id,
        string type
    );
    ACTION claimtool(
        name account,
        uint64_t asset_id
    );
    ACTION claim(
        name account
    );
    ACTION repairbat(
        name account, 
        uint64_t asset_id
    );
    ACTION repairint(
        name account, 
        uint64_t asset_id
    );
    ACTION repairstamina(
        name account, 
        uint16_t stamina
    );
    ACTION addmaterials(
        name account,
        vector<asset> quantities
    );
    ACTION setconfig(
        uint16_t init_stamina,
        uint16_t max_stamina,
        uint16_t init_battery,
        uint16_t max_battery,
        uint16_t carbs_to_stamina,
        uint16_t jigowatts_to_battery,
        uint16_t credits_to_integrity,
        uint16_t reward_rand,
        uint8_t fee
    );
    ACTION settoolconfig(
        uint32_t template_id,
        string tool_name,
        string img,
        name schema_name,
        string type,
        string rarity,
        uint16_t stamina_consumed,
        uint16_t battery_consumed,
        uint16_t integrity_consumed,
        uint16_t cooldown,
        vector<asset> craft,
        vector<asset> rewards
    );
    ACTION setslotconfig(
        uint32_t template_id,
        string slots_name,
        string img,
        name schema_name,
        string rarity,
        uint16_t slots, 
        vector<asset> craft
    );

    ACTION erasetoolconf(
        uint32_t template_id
    );

    ACTION erasetable(
        string table_name
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

    // TODO: remove
    ACTION waste(
        name account, 
        uint64_t asset_id,
        uint16_t battery
    );
    ACTION wastestamina(
        name account,
        uint16_t stamina
    );

private:

    // TABLES 

    // avatars
    TABLE avatars_s {

        name account;
        uint64_t avatar_id;
        
        uint64_t primary_key() const { return account.value; }
    };

    typedef multi_index<name("avatars"), avatars_s> avatars_t;

    avatars_t avatars = avatars_t(get_self(), get_self().value); 

    // struct unclaimed_action
    struct unclaimed_action {
        string game;
        string type;
        asset unclaimed_credits;
    };

    // accounts
    TABLE accounts_s {

        name account;
        uint16_t stamina;
        uint16_t battery;
        vector<asset> balances;
        asset unclaimed_credits;
        vector<unclaimed_action> unclaimed_actions;
        
        uint64_t primary_key() const { return account.value; }
    };

    typedef multi_index<name("accounts"), accounts_s> accounts_t;

    accounts_t accounts = accounts_t(get_self(), get_self().value); 

    // config
    TABLE config_s {

        uint64_t key;    
        uint16_t init_stamina;
        uint16_t max_stamina;
        uint16_t init_battery;
        uint16_t max_battery;
        uint32_t max_unclaimed_credits;
        uint16_t carbs_to_stamina;
        uint16_t jigowatts_to_battery;
        uint16_t credits_to_integrity;
        uint16_t reward_rand;
        uint8_t fee;

        uint64_t primary_key() const { return key; }
    };

    typedef multi_index<name("config"), config_s> config_t;

    config_t config = config_t(get_self(), get_self().value); 

    // tool config
    TABLE toolconfig_s {

        uint32_t template_id;
        string tool_name;
        string img;
        name schema_name;
        string type;
        string rarity;
        uint16_t stamina_consumed;
        uint16_t battery_consumed;
        uint16_t integrity_consumed;
        uint16_t cooldown;
        vector<asset> craft;
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
        uint16_t battery;
        uint16_t max_battery;
        uint16_t integrity;
        uint16_t max_integrity;
        uint64_t last_claim;

        uint64_t primary_key() const { return asset_id; }
        uint64_t by_owner() const { return owner.value; }
    };

    typedef multi_index<name("tools"), tools_s,
        indexed_by < name("byowner"), const_mem_fun < tools_s, uint64_t, &tools_s::by_owner>>> 
    tools_t;

    tools_t tools = tools_t(get_self(), get_self().value);

    // wallet config
    TABLE walletconfig_s {

        uint32_t template_id;
        string wallet_name;
        string img;
        name schema_name;
        string rarity;
        uint32_t capacity;
        uint16_t bonus; 
        vector<asset> craft;

        uint64_t primary_key() const { return template_id; }
    };

    typedef multi_index<name("walletconfig"), walletconfig_s> walletconfig_t;

    walletconfig_t walletconfig = walletconfig_t(get_self(), get_self().value); 

    // wallets
    TABLE wallets_s {

        uint64_t asset_id;
        name owner;
        uint32_t template_id;

        uint64_t primary_key() const { return asset_id; }
        uint64_t by_owner() const { return owner.value; }
    };

    typedef multi_index<name("wallets"), wallets_s,
        indexed_by < name("byowner"), const_mem_fun < wallets_s, uint64_t, &wallets_s::by_owner>>> 
    wallets_t;

    wallets_t wallets = wallets_t(get_self(), get_self().value); 

    // slots config
    TABLE slotsconfig_s {

        uint32_t template_id;
        string slots_name;
        string img;
        name schema_name;
        string rarity;
        uint16_t slots; 
        vector<asset> craft;

        uint64_t primary_key() const { return template_id; }
    };

   typedef multi_index<name("slotsconfig"), slotsconfig_s> slotsconfig_t;

    slotsconfig_t slotsconfig = slotsconfig_t(get_self(), get_self().value); 

    // slots
    TABLE slots_s {

        uint64_t asset_id;
        name owner;
        uint32_t template_id;
        string type;

        uint64_t primary_key() const { return asset_id; }
        uint64_t by_owner() const { return owner.value; }
    };

    typedef multi_index<name("slots"), slots_s,
        indexed_by < name("byowner"), const_mem_fun < slots_s, uint64_t, &slots_s::by_owner>>> 
    slots_t;

    slots_t slots = slots_t(get_self(), get_self().value); 

    // trigger config
    TABLE triggerconfig_s {

        uint32_t template_id;
        string trigger_name;
        string img;
        name schema_name;
        string type;
        string rarity;

        uint64_t primary_key() const { return template_id; }
    };

    typedef multi_index<name("triggconfig"), triggerconfig_s> triggerconfig_t;

    triggerconfig_t triggerconfig = triggerconfig_t(get_self(), get_self().value); 

    // triggers
    TABLE triggers_s {

        uint64_t asset_id;
        name owner;
        string type;
        uint32_t template_id;

        uint64_t primary_key() const { return asset_id; }
    };

    typedef multi_index<name("triggers"), triggers_s> triggers_t;

    triggers_t triggers = triggers_t(get_self(), get_self().value);

    // AUXILIAR FUNCTIONS
    uint64_t finder(vector<asset> assets, symbol symbol); 
    void stakeAvatar(uint64_t asset_ids, name from, name to);
    void stakeTool(uint64_t asset_ids, name from, name to);
    void stakeSlot(uint64_t asset_ids, name from, name to, string type);
    void unstakeTool(name account, uint64_t asset_id);
    void unstakeSlot(name account, uint64_t asset_id);
    void craftTool(name account, uint32_t template_id);
    void craftSlot(name account, uint32_t template_id);
    void checkEmptySlot(string type);

    // CONSTANTS

    const string COLLECTION_NAME = "clashdomewld";
    const string TOOL_SCHEMA_NAME = "tool";
    const string SLOT_SCHEMA_NAME = "slots";
    const string AVATAR_SCHEMA_NAME = "avatar";
    const uint64_t MAX_SLOTS = 3;

    // IN GAME TOKENS
    static constexpr symbol CREDITS_SYMBOL = symbol(symbol_code("CREDITS"), 4);
    static constexpr symbol CARBS_SYMBOL = symbol(symbol_code("CARBS"), 4);
    static constexpr symbol JIGOWATTS_SYMBOL = symbol(symbol_code("WATTS"), 4);

    // BLOCKCHAIN TOKENS
    static constexpr symbol LUDIO_SYMBOL = symbol(symbol_code("LUDIO"), 4);
    static constexpr symbol CDCARBD_SYMBOL = symbol(symbol_code("CDCARBD"), 4);
    static constexpr symbol CDWATTS_SYMBOL = symbol(symbol_code("CDWATTS"), 4);
};