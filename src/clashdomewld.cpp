#include <clashdomewld.hpp>

void clashdomewld::createuser(name owner) {
    
    require_auth(owner);

    auto ac_itr = accounts.find(owner.value);
    check(ac_itr == accounts.end(), "Account with name " + owner.to_string() + " already exists!");

    vector<asset> balances;

    asset ludio;
    ludio.symbol = LUDIO_SYMBOL;
    ludio.amount = 0;
    balances.push_back(ludio);
    
    asset stamina;
    stamina.symbol = STAMINA_SYMBOL;
    stamina.amount = 0;
    balances.push_back(stamina);

    asset xp;
    xp.symbol = XP_SYMBOL;
    xp.amount = 0;
    balances.push_back(xp);

    accounts.emplace(CONTRACTN, [&](auto& account) {
        account.account = owner;
        account.balances = balances;
    });
}

void clashdomewld::unstake(
    name account,
    uint64_t asset_id
) {

    require_auth(account);

    auto tool_itr = tools.find(asset_id);

    check(tool_itr != tools.end(), "Tool with id " + to_string(asset_id) + " doesn't exists!");
    check(tool_itr->owner == account, "Account " + account.to_string() + " isn't the owner of asset " + to_string(asset_id));

    vector<uint64_t> assets;
    assets.push_back(asset_id);

    action(
        permission_level{get_self(), name("active")},
        atomicassets::ATOMICASSETS_ACCOUNT,
        name("transfer"),
        std::make_tuple(
            get_self(),
            account,
            assets,
            "Tool " + to_string(asset_id) + " unstake."
        )
    ).send();

    tools.erase(tool_itr);
}

void clashdomewld::withdraw(
    name account,
    vector<asset> quantities
) {

    require_auth(account);

    auto ac_itr = accounts.find(account.value);
    check(ac_itr != accounts.end(), "Account with name " + account.to_string() + " doesn't exists!");

    auto config_itr = config.begin();   
    uint8_t fee = config_itr->fee;

    for (auto i = 0; i < quantities.size(); i++) {

        uint64_t pos = finder(ac_itr->balances, quantities[i].symbol);

        check(pos != -1, "Invalid symbol.");

        accounts.modify(ac_itr, CONTRACTN, [&](auto& account) {
            account.balances.at(pos) -= quantities[i];
        });

        asset quantity2;
        quantity2.symbol = quantities[i].symbol;
        quantity2.amount = (quantities[i].amount * ((100.0 - fee) / 100.0));

        action(
            permission_level{get_self(), name("active")},
            name("clashdometkn"),
            name("transfer"),
            std::make_tuple(
                get_self(),
                account,
                quantity2,
                "Withdraw " + to_string(quantity2.amount)
            )
        ).send();
    }

}

void clashdomewld::craft(
    name account, 
    uint32_t template_id
) {

    require_auth(account);

    auto ac_itr = accounts.find(account.value);
    check(ac_itr != accounts.end(), "Account with name " + account.to_string() + " doesn't exists!");

    auto tool_itr = toolconfig.find(template_id);
    check(ac_itr != accounts.end(), "Tool with template id " + to_string(template_id) + " doesn't exists!");

    // check all necesary materials && remove materials

    for(auto i = 0; i < tool_itr->mints.size(); i++) {

        uint64_t pos = finder(ac_itr->balances, tool_itr->mints[i].symbol);

        check(pos != -1, "Insufficient materials.");
        check(ac_itr->balances[pos].amount >= tool_itr->mints[i].amount, "Insufficient materials.");

        accounts.modify(ac_itr, CONTRACTN, [&](auto& acc) {
            acc.balances.at(pos) -= tool_itr->mints[i];
        });
    }

    // mint and send tool
    action (
        permission_level{get_self(), name("active")},
        atomicassets::ATOMICASSETS_ACCOUNT,
        name("mintasset"),
        std::make_tuple(
            get_self(),
            name(COLLECTION_NAME),
            name(SCHEMA_NAME),
            template_id,
            account,
            (atomicassets::ATTRIBUTE_MAP) {},
            (atomicassets::ATTRIBUTE_MAP) {},
            (vector <asset>) {}
        )
    ).send();
}

void clashdomewld::claim(
    name account, 
    vector<uint64_t> asset_ids
) {

    require_auth(get_self());
}

void clashdomewld::addmaterials(
    name account,
    vector<asset> quantities
) {

    require_auth(get_self());

    auto ac_itr = accounts.find(account.value);
    check(ac_itr != accounts.end(), "Account with name " + account.to_string() + " doesn't exists!");

    for (auto i = 0; i < quantities.size(); i++) {

        uint64_t pos = finder(ac_itr->balances, quantities[i].symbol);

        check(pos != -1, "Invalid symbol.");

        accounts.modify(ac_itr, CONTRACTN, [&](auto& account) {
            account.balances.at(pos) += quantities[i];
        });
    }

}

void clashdomewld::setconfig(
    uint16_t init_followers, 
    int32_t reward_noise_min, 
    int32_t reward_noise_max,
    uint8_t min_fee,
    uint8_t max_fee,
    uint8_t fee
) {

    require_auth(get_self());

    uint64_t timestamp = eosio::current_time_point().sec_since_epoch();

    auto config_itr = config.begin();
        
    if (config_itr == config.end()) {
        config.emplace(CONTRACTN, [&](auto& config_row) {
            config_row.key = 1;
            config_row.init_followers = init_followers;
            config_row.reward_noise_min = reward_noise_min;
            config_row.reward_noise_max = reward_noise_max;
            config_row.min_fee = min_fee;
            config_row.max_fee = max_fee;
            config_row.last_fee_updated = timestamp;
            config_row.fee = fee;
        });
    } else {
        config.modify(config_itr, CONTRACTN, [&](auto& config_row) {
            config_row.init_followers = init_followers;
            config_row.reward_noise_min = reward_noise_min;
            config_row.reward_noise_max = reward_noise_max;
            config_row.min_fee = min_fee;
            config_row.max_fee = max_fee;
            config_row.last_fee_updated = timestamp;
            config_row.fee = fee;
        });
    }
}

void clashdomewld::settoolconfig(
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
) {
    require_auth(get_self());

    auto config_itr = toolconfig.find(template_id);

    if (config_itr == toolconfig.end()) {
        toolconfig.emplace(CONTRACTN, [&](auto& config_row) {
            config_row.template_id = template_id;
            config_row.tool_name = tool_name;
            config_row.img = img;
            config_row.schema_name = schema_name;
            config_row.type = type;
            config_row.rarity = rarity;
            config_row.level = level;
            config_row.stamina_consumed = stamina_consumed;
            config_row.experience_consumed = experience_consumed;
            config_row.mints = mints;
            config_row.rewards = rewards;
        });
    } else {
        toolconfig.modify(config_itr, CONTRACTN, [&](auto& config_row) {
            config_row.tool_name = tool_name;
            config_row.img = img;
            config_row.schema_name = schema_name;
            config_row.type = type;
            config_row.rarity = rarity;
            config_row.level = level;
            config_row.stamina_consumed = stamina_consumed;
            config_row.experience_consumed = experience_consumed;
            config_row.mints = mints;
            config_row.rewards = rewards;
        });
    }
}

void clashdomewld::receive_asset_transfer(
    name from,
    name to,
    vector <uint64_t> asset_ids,
    string memo
) {

    if (to != get_self()) {
        return;
    }

    check(memo == "stake", "Invalid memo.");

    check(asset_ids.size() == 1, "Only one tool can be sent at a time.");

    atomicassets::assets_t player_assets = atomicassets::get_assets(to);
    auto asset_itr = player_assets.require_find(asset_ids[0], "No NFT with this ID exists");

    // CHECK THAT THE ASSET CORRESPONDS TO OUR COLLECTION
    check(asset_itr->collection_name == name(COLLECTION_NAME), "NFT doesn't correspond to " + COLLECTION_NAME);
    check(asset_itr->schema_name == name(SCHEMA_NAME), "NFT doesn't correspond to schema " + SCHEMA_NAME);

    atomicassets::schemas_t collection_schemas = atomicassets::get_schemas(name(COLLECTION_NAME));
    auto schema_itr = collection_schemas.find(name(SCHEMA_NAME).value);

    atomicassets::templates_t collection_templates = atomicassets::get_templates(name(COLLECTION_NAME));
    auto template_itr = collection_templates.find(asset_itr->template_id);

    vector <uint8_t> immutable_serialized_data = template_itr->immutable_serialized_data;

    atomicassets::ATTRIBUTE_MAP idata = atomicdata::deserialize(immutable_serialized_data, schema_itr->format);

    tools.emplace(CONTRACTN, [&](auto& tool) {
        tool.asset_id = asset_ids[0];
        tool.owner = from;
        tool.type = get<string> (idata["type"]);
        tool.template_id = asset_itr->template_id;
    });
    
}

void clashdomewld::receive_token_transfer(
    name from,
    name to,
    asset quantity,
    string memo
) {

    if (to != get_self()) {
        return;
    }

    if(memo == "deposit") {
        auto ac_itr = accounts.find(from.value);
        check(ac_itr != accounts.end(), "Account with name " + from.to_string() + " doesn't exists!");

        if (quantity.symbol == LUDIO_SYMBOL) {

            uint64_t pos = finder(ac_itr->balances, LUDIO_SYMBOL);

            check(pos != -1, "Invalid symbol.");

            accounts.modify(ac_itr, CONTRACTN, [&](auto& account) {
                account.balances.at(pos) += quantity;
            });
        }
    } else {
        check(memo == "transfer", "Invalid memo.");
    }

    
}

uint64_t clashdomewld::finder(vector<asset> assets, symbol symbol)
{
    for (uint64_t i = 0; i < assets.size(); i++)
    {
        if (assets.at(i).symbol == symbol)
        {
            return i;
        }
    }
    return -1;
}
