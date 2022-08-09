#include <eosio/eosio.hpp>
#include <eosio/binary_extension.hpp>
#include <eosio/asset.hpp>
#include <eosio/crypto.hpp>
#include <eosio/transaction.hpp>
#include <atomicassets.hpp>
#include <atomicdata.hpp>
#include <string>
#include <nlohmann/json.hpp>

using namespace eosio;
using namespace std;

using json = nlohmann::json;

#define EOSIO name("eosio")
#define CONTRACTN name("clashdomewld")

CONTRACT clashdomewld : public contract {

public:

    using contract::contract;

    ACTION staketrial(
        name account,
        uint64_t asset_id
    );
    ACTION unstake(
        name account,
        uint64_t asset_id,
        string type
    );
    ACTION withdraw(
        name account,
        vector<asset> quantities
    );
    ACTION claimtool(
        name account,
        uint64_t asset_id
    );
    ACTION claim(
        name account
    );
    ACTION claimtrial(
        name account,
        name afiliate
    );
    ACTION claimhalltr(
        name account,
        uint64_t asset_id
    );
    ACTION addcredits(
        name account,
        asset credits,
        vector<string> unclaimed_actions,
        const string&  memo 
    );
    ACTION addaffiliate(
        name account,
        uint8_t commission,
        uint16_t available_trials
    );
    ACTION editsocial(
        name account,
        string memo
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
    ACTION setshopitem(
        uint32_t template_id,
        string item_name,
        string img,
        name schema_name,
        string game,
        uint64_t timestamp_start,
        uint64_t timestamp_end,
        int32_t max_claimable,
        int32_t available_items,
        int32_t account_limit,
        vector<asset> price,
        string description,
        string extra_data
    );
    ACTION eraseshopit(
        uint32_t template_id
    );
    ACTION addtowl(
        uint32_t template_id,
        vector<name> accounts_to_add
    );
    ACTION erasefromwl(
        uint32_t template_id,
        name account_to_remove
    );
    ACTION clearwl(
        uint32_t template_id
    );
    ACTION setaclimitwl(
        uint32_t template_id,
        int32_t account_limit
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
    ACTION setcraft(
        uint32_t template_id,
        vector<asset> craft
    );ACTION setslotcraft(
        uint32_t template_id,
        vector<asset> craft
    );
    ACTION setreward(
        uint32_t template_id,
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
    ACTION eraseaccount(
        name account
    );
    ACTION erasecitiz(
        name account
    );
    ACTION erasetrial(
        name account
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
        asset unclaimed_credits,
        vector<string> unclaimed_actions
    );
    ACTION settrialcr(
        name account,
        asset credits
    );
    ACTION setdecdata(
        uint64_t asset_id,
        name account,
        string data
    );
    ACTION addcitizen(
        name account,   
        uint8_t type,
        uint64_t citizen_id
    );
    ACTION addaccount(
        name account
    );
    ACTION initgsconf(
    );
    ACTION receiverand(
        uint64_t assoc_id,
        checksum256 random_value
    );
    ACTION addavatar(
        name acount,
        string avatar
    );
    
    ACTION loggigaswap(
        name acount,
        vector<uint8_t> player_choices,
        name rival,
        vector<uint8_t> rival_choices,
        uint64_t random_value,
        int8_t points,
        name winner
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

    [[eosio::on_notify("eosio.token::transfer")]] void receive_wax_transfer(
        name from,
        name to,
        asset quantity,
        string memo
    );

    [[eosio::on_notify("clashdometkn::transfers")]] void receive_tokens_transfer(
        name from,
        name to,
        vector <asset> quantities,
        string memo
    );

private:

    // TABLES 

    // social
    TABLE social_s {
        
        name account;
        string data;      

        uint64_t primary_key() const { return account.value; }
    };

    typedef multi_index<name("social"), social_s> social_t;
    
    social_t social = social_t(get_self(), get_self().value);

    // citiz
    TABLE citiz_s {

        name account;
        uint8_t type;
        uint64_t citizen_id;
        string avatar;
        
        uint64_t primary_key() const { return account.value; }
    };

    typedef multi_index<name("citiz"), citiz_s> citiz_t;

    citiz_t citiz = citiz_t(get_self(), get_self().value); 

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

    // trials
    TABLE trials_s {

        name account;
        uint64_t asset_id;
        asset credits;
        vector<string> unclaimed_actions;
        bool staked;
        bool full;
        
        uint64_t primary_key() const { return account.value; }
    };

    typedef multi_index<name("trials"), trials_s> trials_t;

    trials_t trials = trials_t(get_self(), get_self().value); 

    // partners
    struct earning {
        uint64_t timestamp;      
        uint64_t duel_id;  
        asset fee; 
    };

    TABLE partners_s {

        name account;
        name partner;
        vector<earning> unlcaimed_earnings;
        vector<earning> claimed_earnings;
        
        uint64_t primary_key() const { return account.value; }
        uint64_t by_partner() const { return partner.value; }
    };

    typedef multi_index<name("partners"), partners_s,
        indexed_by < name("bypartner"), const_mem_fun < partners_s, uint64_t, &partners_s::by_partner>>>
    partners_t;

    partners_t partners = partners_t(get_self(), get_self().value);

    // affiliates

    TABLE affiliates_s {

        name account;
        uint8_t commission;
        uint16_t available_trials;
        
        uint64_t primary_key() const { return account.value; }
    };

    typedef multi_index<name("affiliates"), affiliates_s> affiliates_t;

    affiliates_t affiliates = affiliates_t(get_self(), get_self().value); 

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

    // shop
    TABLE shop_s {
        uint32_t template_id;
        string item_name;
        string img;
        name schema_name;
        string game;
        uint64_t timestamp_start;
        uint64_t timestamp_end;
        int32_t max_claimable;
        int32_t available_items;
        int32_t account_limit;
        vector<asset> price;
        string description;
        string extra_data;
        vector<name> whitelist;

        uint64_t primary_key() const { return template_id; }
    };

    typedef multi_index<name("shop"), shop_s> shop_t;

    shop_t shop = shop_t(get_self(), get_self().value); 

    // shop_max_claimable
    TABLE smclaim_s {
        name account;
        uint32_t template_id;
        uint32_t claims;

        uint64_t primary_key() const { return account.value; }
    };

    typedef multi_index<name("smclaim"), smclaim_s> smclaim_t;

    smclaim_t smclaim = smclaim_t(get_self(), get_self().value); 

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

    // gigaswap config
    TABLE gigasconfig_s {

        uint8_t key;   
        vector<uint8_t> choices;
        name account;
        
        uint64_t primary_key() const { return key; }
    };

    typedef multi_index<name("gigasconfig"), gigasconfig_s> gigasconfig_t;

    gigasconfig_t gigasconfig = gigasconfig_t(get_self(), get_self().value);

    // gigaswap config  2
    TABLE gigasconfig2_s {

        uint8_t key;   
        vector<uint8_t> choices;
        vector<name> accounts;
        
        uint64_t primary_key() const { return key; }
    };

    typedef multi_index<name("gigasconfig2"), gigasconfig2_s> gigasconfig2_t;

    gigasconfig2_t gigasconfig2 = gigasconfig2_t(get_self(), get_self().value);

    // gigaswap

    TABLE gigaswap_s {

        name account;
        uint64_t timestamp;
        vector<uint8_t> choices;
        vector<asset> quantities;
        uint8_t status;
        name opponent; 
        vector<uint8_t> opponent_choices;  
        name winner;
        
        uint64_t primary_key() const { return account.value; }
    };

    typedef multi_index<name("gigaswap"), gigaswap_s> gigaswap_t;

    gigaswap_t gigaswap = gigaswap_t(get_self(), get_self().value);

    //stats
    TABLE tokenstats_s{

        uint32_t day;
        asset mined_carbz;
        asset consumed_carbz;
        asset burned_carbz;
        asset mined_credits;
        asset consumed_credits;
        asset burned_credits;
        asset mined_jigo;
        asset consumed_jigo;
        asset burned_jigo;

        uint64_t primary_key() const {return (uint64_t) day;}
    };

    typedef multi_index<name("tokenstats"), tokenstats_s> coindailystats_t;

    coindailystats_t tokenstats = coindailystats_t(get_self(), get_self().value); 

    // decorations
    TABLE decorations_s {
        uint64_t asset_id;
        name owner;
        uint32_t template_id;
        string data;

        uint64_t primary_key() const { return asset_id; }
        uint64_t by_owner() const { return owner.value; }
    };

    typedef multi_index<name("decorations"), decorations_s,
        indexed_by < name("byowner"), const_mem_fun < decorations_s, uint64_t, &decorations_s::by_owner>>> 
    decorations_t;

    decorations_t decorations = decorations_t(get_self(), get_self().value); 

    // AUXILIAR FUNCTIONS
    uint64_t finder(vector<asset> assets, symbol symbol); 
    uint64_t finder(vector<name> whitelist, name account); 
    void stakeAvatar(uint64_t asset_ids, name from, name to, string memo);
    void stakeTool(uint64_t asset_ids, name from, name to);
    void stakeWallet(uint64_t asset_ids, name from, name to);
    void stakeSlot(uint64_t asset_ids, name from, name to, string type);
    void stakeDecoration(uint64_t asset_ids, name from, name to);
    void getTokens(uint64_t asset_ids, name from, name to);
    void burnTokens(asset tokens, string memo_extra);
    void burnTrial(name account);
    void checkEarlyAccess(name account, uint64_t early_access);
    void parseSocialsMemo(name account, string memo);
    void updateDailyStats(asset assetVal,int type);
    symbol tokenConversion(symbol s1);
    uint32_t epochToDay(time_t time);

    // CONSTANTS

    // const string COLLECTION_NAME = "clashdomewld";
    const string COLLECTION_NAME = "clashdomenft";

    const uint32_t EARLY_ACCESS_TEMPLATE_ID = 230544;
    const string TOOL_SCHEMA_NAME = "tool";
    const string SLOT_SCHEMA_NAME = "slot";
    const string WALLET_SCHEMA_NAME = "wallet";
    const string DECORATION_SCHEMA_NAME = "decoration";
    const string CITIZEN_SCHEMA_NAME = "citizen";
    const string HALLS_SCHEMA_NAME = "poolhalls";
    const string PACKS_SCHEMA_NAME = "packs";

    // mainnet
    const uint32_t PACKS_TEMPLATE_ID = 373360;
    const uint32_t TRIAL_TEMPLATE_ID = 530445;

    // testnet
    // const uint32_t PACKS_TEMPLATE_ID = 403495;
    // const uint32_t TRIAL_TEMPLATE_ID = 447908;

    const uint64_t PACK_CARBZ_REWARD = 15000000;
    const uint64_t PACK_JIGO_REWARD = 10000000;
    const uint64_t SOCIAL_CARBZ_PAYMENT = 3500000;
    const uint64_t TRIAL_MAX_UNCLAIMED = 3600000;
    const uint64_t MAX_SLOTS = 3;
    const uint64_t CRAFT_BURN_PERCENT = 10;
    enum CitizenType {PLEB = 0, UBERENORM, HIGH_CLONE};

    const string CUSTOM_NAME = "cn";
    const string COUNTRY = "co";
    const string TWITTER = "tw";
    const string TELEGRAM = "tg";
    const string DISCORD = "dc";

    enum ChoiceType {ROCK = 0, PAPER, SCISSORS};
    enum StatusType {PENDING = 0, DONE};

    const int8_t LOSE = -1;
    const int8_t TIE = 0;
    const int8_t WIN = 1;
    const vector<vector<int8_t>> GIGASWAP_MAP = {{TIE, LOSE, WIN}, {WIN, TIE, LOSE}, {LOSE, WIN, TIE}};

    // IN GAME TOKENS
    static constexpr symbol CREDITS_SYMBOL = symbol(symbol_code("CREDITS"), 4);
    static constexpr symbol CARBZ_SYMBOL = symbol(symbol_code("CARBZ"), 4);
    static constexpr symbol JIGOWATTS_SYMBOL = symbol(symbol_code("JIGO"), 4);

    // BLOCKCHAIN TOKENS
    static constexpr symbol LUDIO_SYMBOL = symbol(symbol_code("LUDIO"), 4);
    static constexpr symbol CDCARBZ_SYMBOL = symbol(symbol_code("CDCARBZ"), 4);
    static constexpr symbol CDJIGO_SYMBOL = symbol(symbol_code("CDJIGO"), 4);
};