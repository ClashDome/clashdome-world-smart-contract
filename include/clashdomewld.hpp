#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/transaction.hpp>
#include <atomicassets.hpp>
#include <atomicdata.hpp>
#include <string>

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
    ACTION addcredits(
        name account,
        asset credits,
        vector<string> unclaimed_actions
    );
    ACTION repairbat(
        name account, 
        uint64_t asset_id
    );
    ACTION repairaccbat(
        name account
    );
    ACTION repairint(
        name account, 
        uint64_t asset_id
    );
    ACTION repairstamina(
        name account
    );
    ACTION addmaterials(
        name account,
        vector<asset> quantities
    );
    ACTION setconfig(
        uint16_t init_battery,
        uint16_t max_battery,
        uint32_t max_unclaimed_credits,
        uint16_t carbz_to_stamina,
        uint16_t jigowatts_to_battery,
        uint16_t credits_to_integrity,
        uint16_t wallet_stamina_consumed,
        uint16_t wallet_battery_consumed
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
        uint16_t battery,
        uint16_t integrity,
        uint16_t cooldown,
        vector<asset> craft,
        vector<asset> rewards
    );
    ACTION setcooldown(
        uint32_t template_id,
        uint16_t cooldown
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
    ACTION setciticonfig(
        uint8_t key,   
        string type,
        uint16_t max_stamina,
        uint8_t extra_claim
    );
    ACTION setwalletconf(
        uint32_t template_id,
        string wallet_name,
        string img,
        name schema_name,
        string rarity,
        uint32_t extra_capacity,
        uint16_t battery_consumed,
        uint16_t stamina_consumed,
        vector<asset> craft
    );
    ACTION erasetoolconf(
        uint32_t template_id
    );
    ACTION erasetable(
        string table_name
    );
    ACTION setaccvalues(
        name account,
        uint16_t stamina,
        uint16_t battery,
        uint16_t carbz_slots,
        uint16_t jigowatts_slots,
        uint16_t carbz_free_slots,
        uint16_t jigowatts_free_slots,
        vector<asset> balances,
        asset unclaimed_credits,
        vector<string> unclaimed_actions
    );
    ACTION addcitizen(
        name account,   
        uint8_t type,
        uint64_t citizen_id
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

    // citizens
    TABLE citizens_s {

        name account;
        uint8_t type;
        uint64_t citizen_id;
        
        uint64_t primary_key() const { return account.value; }
    };

    typedef multi_index<name("citizens"), citizens_s> citizens_t;

    citizens_t citizens = citizens_t(get_self(), get_self().value); 

    // accounts
    TABLE accounts_s {

        name account;
        uint16_t stamina;
        uint16_t battery;
        uint16_t carbz_slots;
        uint16_t jigowatts_slots;
        uint16_t carbz_free_slots;
        uint16_t jigowatts_free_slots;
        vector<asset> balances;
        asset unclaimed_credits;
        vector<string> unclaimed_actions;
        
        uint64_t primary_key() const { return account.value; }
    };

    typedef multi_index<name("accounts"), accounts_s> accounts_t;

    accounts_t accounts = accounts_t(get_self(), get_self().value); 

    // config
    TABLE config_s {

        uint64_t key;
        uint16_t init_battery;
        uint16_t max_battery;
        uint32_t max_unclaimed_credits;
        uint16_t carbz_to_stamina;
        uint16_t jigowatts_to_battery;
        uint16_t credits_to_integrity;
        uint16_t wallet_stamina_consumed;
        uint16_t wallet_battery_consumed;

        uint64_t primary_key() const { return key; }
    };

    typedef multi_index<name("config"), config_s> config_t;

    config_t config = config_t(get_self(), get_self().value); 

    // citizen config
    TABLE citizconfig_s {

        uint8_t key;    
        string type;
        uint16_t max_stamina;
        uint8_t claim_extra;

        uint64_t primary_key() const { return key; }
    };

    typedef multi_index<name("citizconfig"), citizconfig_s> citizconfig_t;

    citizconfig_t citizconfig = citizconfig_t(get_self(), get_self().value); 

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
        uint16_t battery;
        uint16_t integrity;
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
        uint32_t extra_capacity;
        uint16_t battery_consumed;
        uint16_t stamina_consumed;
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
    void stakeAvatar(uint64_t asset_ids, name from, name to, string memo);
    void stakeTool(uint64_t asset_ids, name from, name to);
    void stakeWallet(uint64_t asset_ids, name from, name to);
    void stakeSlot(uint64_t asset_ids, name from, name to, string type);
    void craftTool(name account, uint32_t template_id);
    void craftSlot(name account, uint32_t template_id);
    void craftWallet(name account, uint32_t template_id);
    void getTokens(uint64_t asset_ids, name from, name to);
    void burnTokens(asset tokens, string memo_extra);
    void checkEarlyAccess(name account, uint64_t early_access);

    // CONSTANTS

    // TODO: change for production

    // const string COLLECTION_NAME = "clashdomewld";
    const string COLLECTION_NAME = "clashdomenft";

    const uint32_t EARLY_ACCESS_TEMPLATE_ID = 230544;
    const string TOOL_SCHEMA_NAME = "tool";
    const string SLOT_SCHEMA_NAME = "slot";
    const string WALLET_SCHEMA_NAME = "wallet";
    const string CITIZEN_SCHEMA_NAME = "citizen";
    const string PACKS_SCHEMA_NAME = "packs";
    const uint32_t PACKS_TEMPLATE_ID = 373360;
    const uint64_t PACK_CARBZ_REWARD = 15000000;
    const uint64_t PACK_JIGO_REWARD = 10000000;
    const uint64_t MAX_SLOTS = 3;
    const uint64_t CRAFT_BURN_PERCENT = 10;
    enum CitizenType {PLEB = 0, UBERENORM, HIGH_CLONE};

    // IN GAME TOKENS
    static constexpr symbol CREDITS_SYMBOL = symbol(symbol_code("CREDITS"), 4);
    static constexpr symbol CARBZ_SYMBOL = symbol(symbol_code("CARBZ"), 4);
    static constexpr symbol JIGOWATTS_SYMBOL = symbol(symbol_code("JIGO"), 4);

    // BLOCKCHAIN TOKENS
    static constexpr symbol LUDIO_SYMBOL = symbol(symbol_code("LUDIO"), 4);
    static constexpr symbol CDCARBZ_SYMBOL = symbol(symbol_code("CDCARBZ"), 4);
    static constexpr symbol CDJIGO_SYMBOL = symbol(symbol_code("CDJIGO"), 4);
};