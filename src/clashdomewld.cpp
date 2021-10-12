#include <clashdomewld.hpp>

void clashdomewld::unstake(
    name account,
    uint64_t asset_id,
    string type
) {

    require_auth(account);

    if (type == TOOL_SCHEMA_NAME) {
        auto tool_itr = tools.find(asset_id);

        check(tool_itr != tools.end(), "Tool with id " + to_string(asset_id) + " doesn't exists!");
        check(tool_itr->owner == account, "Account " + account.to_string() + " isn't the owner of asset " + to_string(asset_id));

        auto tool_conf_itr = toolconfig.find(tool_itr->template_id);
        check(tool_conf_itr != toolconfig.end(), "Tool with template id " + to_string(tool_itr->template_id) + " doesn't exists!");

        uint64_t timestamp = eosio::current_time_point().sec_since_epoch();

        check(tool_itr->last_claim + tool_conf_itr->cooldown <= timestamp, "Claim not available yet. Wait until you have the tool claimeable.");
        check(tool_itr->battery == tool_itr->max_battery, "You need to charge battery!");
        check(tool_itr->integrity == tool_itr->max_integrity, "You need to charge integrity!");

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
    } else if (type == SLOT_SCHEMA_NAME) {

        auto slot_itr = slots.find(asset_id);

        check(slot_itr != slots.end(), "Tool with id " + to_string(asset_id) + " doesn't exists!");
        check(slot_itr->owner == account, "Account " + account.to_string() + " isn't the owner of asset " + to_string(asset_id));

        checkEmptySlot(slot_itr->type);

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
                "Slot " + to_string(asset_id) + " unstake."
            )
        ).send();

        slots.erase(slot_itr);
    }
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
    uint32_t template_id,
    string type
) {

    require_auth(account);

    if (type == TOOL_SCHEMA_NAME) {
        craftTool(account, template_id);
    } else if (type == SLOT_SCHEMA_NAME) {
        craftSlot(account, template_id);
    }
}

void clashdomewld::repairbat(
    name account,
    uint64_t asset_id
) {

    require_auth(account);

    auto config_itr = config.begin();

    auto ac_itr = accounts.find(account.value);
    check(ac_itr != accounts.end(), "Account with name " + account.to_string() + " doesn't exists!");

    auto tool_itr = tools.find(asset_id);
    check(tool_itr != tools.end(), "Tool with id " + to_string(asset_id) + " doesn't exists!");
    
    check(tool_itr->owner == account, "Account " + account.to_string() + " isn't the owner of asset " + to_string(asset_id));

    auto tool_conf_itr = toolconfig.find(tool_itr->template_id);
    check(tool_conf_itr != toolconfig.end(), "Tool with template id " + to_string(tool_itr->template_id) + " doesn't exists!");

    uint64_t pos = finder(ac_itr->balances, JIGOWATTS_SYMBOL);
    check(pos != -1, "Invalid symbol.");

    uint16_t battery_necessary = tool_itr->max_battery - tool_itr->battery;

    check(battery_necessary > 0, "No need to repair Battery.");

    check(ac_itr->balances[pos].amount / config_itr->jigowatts_to_battery >= battery_necessary * 10000, "Insufficient Jigowatts.");

    tools.modify(tool_itr, CONTRACTN, [&](auto& tool) {
        tool.battery += battery_necessary;
    });

    accounts.modify(ac_itr, CONTRACTN, [&](auto& acc) {
        acc.balances.at(pos).amount -= (battery_necessary * 10000) / config_itr->jigowatts_to_battery;
    });
}

void clashdomewld::repairint(
    name account,
    uint64_t asset_id
) {

    require_auth(account);

    auto config_itr = config.begin();

    auto ac_itr = accounts.find(account.value);
    check(ac_itr != accounts.end(), "Account with name " + account.to_string() + " doesn't exists!");

    auto tool_itr = tools.find(asset_id);
    check(tool_itr != tools.end(), "Tool with id " + to_string(asset_id) + " doesn't exists!");
    
    check(tool_itr->owner == account, "Account " + account.to_string() + " isn't the owner of asset " + to_string(asset_id));

    auto tool_conf_itr = toolconfig.find(tool_itr->template_id);
    check(tool_conf_itr != toolconfig.end(), "Tool with template id " + to_string(tool_itr->template_id) + " doesn't exists!");

    uint64_t pos = finder(ac_itr->balances, CREDITS_SYMBOL);
    check(pos != -1, "Invalid symbol.");

    uint16_t integrity_necessary = tool_itr->max_integrity - tool_itr->integrity;

    check(integrity_necessary > 0, "No need to repair Integrity.");
    
    check(ac_itr->balances[pos].amount / config_itr->credits_to_integrity >= integrity_necessary * 10000, "Insufficient Credits.");

    tools.modify(tool_itr, CONTRACTN, [&](auto& tool) {
        tool.integrity += integrity_necessary;
    });

    accounts.modify(ac_itr, CONTRACTN, [&](auto& acc) {
        acc.balances.at(pos).amount -= (integrity_necessary * 10000) / config_itr->credits_to_integrity;
    });
}

void clashdomewld::repairstamina(
    name account, 
    uint16_t stamina
) {

    check(stamina > 0, "Only positive values.");

    require_auth(account);

    auto config_itr = config.begin();

    auto ac_itr = accounts.find(account.value);
    check(ac_itr != accounts.end(), "Account with name " + account.to_string() + " doesn't exists!");

    uint64_t pos = finder(ac_itr->balances, CARBS_SYMBOL);
    check(pos != -1, "Invalid symbol.");

    check(stamina + ac_itr->stamina <= config_itr->max_stamina, "Too much stamina.");
    check(ac_itr->balances[pos].amount / config_itr->carbs_to_stamina >= stamina * 10000, "Insufficient Stamina.");

    accounts.modify(ac_itr, CONTRACTN, [&](auto& acc) {
        acc.stamina += stamina;
        acc.balances.at(pos).amount -= (stamina * 10000) / config_itr->carbs_to_stamina;
    });
}

void clashdomewld::claimtool(
    name account,
    uint64_t asset_id
) {

    require_auth(account);

    // check cooldown

    auto ac_itr = accounts.find(account.value);
    check(ac_itr != accounts.end(), "Account with name " + account.to_string() + " doesn't exists!");

    auto tool_itr = tools.find(asset_id);

    check(tool_itr != tools.end(), "Tool with id " + to_string(asset_id) + " doesn't exists!");
    check(tool_itr->owner == account, "Account " + account.to_string() + " isn't the owner of asset " + to_string(asset_id));

    auto tool_conf_itr = toolconfig.find(tool_itr->template_id);
    check(tool_conf_itr != toolconfig.end(), "Tool with template id " + to_string(tool_itr->template_id) + " doesn't exists!");

    uint64_t timestamp = eosio::current_time_point().sec_since_epoch();

    check(tool_itr->last_claim + tool_conf_itr->cooldown <= timestamp, "Claim not available yet.");
    check(ac_itr->stamina - tool_conf_itr->stamina_consumed >= 0, "Insufficient stamina.");
    check(tool_itr->battery - tool_conf_itr->battery_consumed >= 0, "Insufficient battery.");
    check(tool_itr->integrity - tool_conf_itr->integrity_consumed >= 0, "Insufficient integrity.");

    uint64_t pos = finder(ac_itr->balances, tool_conf_itr->rewards[0].symbol);

    accounts.modify(ac_itr, CONTRACTN, [&](auto& acc) {
        acc.stamina -= tool_conf_itr->stamina_consumed;
        acc.balances.at(pos).amount += tool_conf_itr->rewards[0].amount;
    });

    tools.modify(tool_itr, CONTRACTN, [&](auto& tool) {
        tool.battery -= tool_conf_itr->battery_consumed;
        tool.integrity -= tool_conf_itr->integrity_consumed;
        tool.last_claim = timestamp;
    });
}

void clashdomewld::claim(
    name account
) {

    require_auth(account);
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
    uint16_t init_stamina,
    uint16_t max_stamina,
    uint16_t init_battery,
    uint16_t max_battery,
    uint16_t carbs_to_stamina,
    uint16_t jigowatts_to_battery,
    uint16_t credits_to_integrity,
    uint16_t reward_rand,
    uint8_t fee
) {

    require_auth(get_self());

    uint64_t timestamp = eosio::current_time_point().sec_since_epoch();

    auto config_itr = config.begin();
        
    if (config_itr == config.end()) {
        config.emplace(CONTRACTN, [&](auto& config_row) {
            config_row.key = 1;
            config_row.init_stamina = init_stamina;
            config_row.max_stamina = max_stamina;
            config_row.init_battery = init_battery;
            config_row.max_battery = max_battery;
            config_row.carbs_to_stamina = carbs_to_stamina;
            config_row.jigowatts_to_battery = jigowatts_to_battery;
            config_row.credits_to_integrity = credits_to_integrity;
            config_row.reward_rand = reward_rand;
            config_row.fee = fee;
        });
    } else {
        config.modify(config_itr, CONTRACTN, [&](auto& config_row) {
            config_row.init_stamina = init_stamina;
            config_row.max_stamina = max_stamina;
            config_row.init_battery = init_battery;
            config_row.max_battery = max_battery;
            config_row.carbs_to_stamina = carbs_to_stamina;
            config_row.jigowatts_to_battery = jigowatts_to_battery;
            config_row.credits_to_integrity = credits_to_integrity;
            config_row.reward_rand = reward_rand;
            config_row.fee = fee;
        });
    }
}

void clashdomewld::settoolconfig(
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
            config_row.stamina_consumed = stamina_consumed;
            config_row.battery_consumed = battery_consumed;
            config_row.integrity_consumed = integrity_consumed;
            config_row.cooldown = cooldown;
            config_row.craft = craft;
            config_row.rewards = rewards;
        });
    } else {
        toolconfig.modify(config_itr, CONTRACTN, [&](auto& config_row) {
            config_row.tool_name = tool_name;
            config_row.img = img;
            config_row.schema_name = schema_name;
            config_row.type = type;
            config_row.rarity = rarity;
            config_row.stamina_consumed = stamina_consumed;
            config_row.battery_consumed = battery_consumed;
            config_row.integrity_consumed = integrity_consumed;
            config_row.cooldown = cooldown;
            config_row.craft = craft;
            config_row.rewards = rewards;
        });
    }
}

void clashdomewld::setslotconfig(
    uint32_t template_id,
    string slots_name,
    string img,
    name schema_name,
    string rarity,
    uint16_t slots, 
    vector<asset> craft
) {
    require_auth(get_self());

    auto config_itr = slotsconfig.find(template_id);

    if (config_itr == slotsconfig.end()) {
        slotsconfig.emplace(CONTRACTN, [&](auto& config_row) {
            config_row.template_id = template_id;
            config_row.slots_name = slots_name;
            config_row.img = img;
            config_row.schema_name = schema_name;
            config_row.rarity = rarity;
            config_row.slots = slots;
            config_row.craft = craft;
        });
    } else {
        slotsconfig.modify(config_itr, CONTRACTN, [&](auto& config_row) {
            config_row.template_id = template_id;
            config_row.slots_name = slots_name;
            config_row.img = img;
            config_row.schema_name = schema_name;
            config_row.rarity = rarity;
            config_row.slots = slots;
            config_row.craft = craft;
        });
    }
}

void clashdomewld::erasetoolconf(
    uint32_t template_id
) {

    require_auth(get_self());

    auto config_itr = toolconfig.find(template_id);

    check(config_itr != toolconfig.end(), "Tool with template " + to_string(template_id) + " doesn't exists!");

    toolconfig.erase(config_itr);
}

void clashdomewld::erasetable(
    string table_name
) {

    require_auth(get_self());

    if (table_name == "accounts") {
        for (auto itr = accounts.begin(); itr != accounts.end();) {
            itr = accounts.erase(itr);
        }
    } else if (table_name == "toolconfig") {
        for (auto itr = toolconfig.begin(); itr != toolconfig.end();) {
            itr = toolconfig.erase(itr);
        }
    } else if (table_name == "tools") {
        for (auto itr = tools.begin(); itr != tools.end();) {
            itr = tools.erase(itr);
        }
    } else if (table_name == "config") {
        for (auto itr = config.begin(); itr != config.end();) {
            itr = config.erase(itr);
        }
    } else if (table_name == "slots") {
        for (auto itr = slots.begin(); itr != slots.end();) {
            itr = slots.erase(itr);
        }
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

    auto ac_itr = accounts.find(from.value);
    check(ac_itr != accounts.end(), "Account with name " + from.to_string() + " doesn't exists!");

    check(asset_ids.size() == 1, "Only one tool can be sent at a time.");

    if (memo == "stake tool") {

        stakeTool(asset_ids[0], from, to);

    } else if (memo == "stake avatar") {

        stakeAvatar(asset_ids[0], from, to);

    } else if (memo.find("stake slots") != string::npos) {

        if (memo.find("Carbs") != string::npos) {
            stakeSlot(asset_ids[0], from, to, "Carbs");
        } else if (memo.find("Jigowatts") != string::npos) {
            stakeSlot(asset_ids[0], from, to, "Jigowatts");
        }
    }
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

            uint64_t pos = finder(ac_itr->balances, CREDITS_SYMBOL);

            check(pos != -1, "Invalid symbol.");

            accounts.modify(ac_itr, CONTRACTN, [&](auto& account) {
                account.balances.at(pos) += quantity;
            });
        } else if (quantity.symbol == CDCARBD_SYMBOL) {

            uint64_t pos = finder(ac_itr->balances, CARBS_SYMBOL);

            check(pos != -1, "Invalid symbol.");

            accounts.modify(ac_itr, CONTRACTN, [&](auto& account) {
                account.balances.at(pos) += quantity;
            });
        } else if (quantity.symbol == CDWATTS_SYMBOL) {

            uint64_t pos = finder(ac_itr->balances, JIGOWATTS_SYMBOL);

            check(pos != -1, "Invalid symbol.");

            accounts.modify(ac_itr, CONTRACTN, [&](auto& account) {
                account.balances.at(pos) += quantity;
            });
        }
    } else {
        check(memo == "transfer", "Invalid memo.");
    }
}

void clashdomewld::waste(
    name account,
    uint64_t asset_id,
    uint16_t battery
) {
    require_auth(get_self());

    auto tool_itr = tools.find(asset_id);
    check(tool_itr != tools.end(), "Tool with id " + to_string(asset_id) + " doesn't exists!");

    check(tool_itr->battery >= battery, "The tool doesn't have as much battery.");

    tools.modify(tool_itr, CONTRACTN, [&](auto& tool) {
        tool.battery -= battery;
    });
}

void clashdomewld::wastestamina(
    name account,
    uint16_t stamina
) {
    require_auth(get_self());

    auto config_itr = config.begin();

    auto ac_itr = accounts.find(account.value);
    check(ac_itr != accounts.end(), "Account with name " + account.to_string() + " doesn't exists!");

    check(ac_itr->stamina >= stamina, "Insufficient stamina.");

    accounts.modify(ac_itr, CONTRACTN, [&](auto& acc) {
        acc.stamina -= stamina;
    });
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

void clashdomewld::stakeAvatar(uint64_t asset_id, name from, name to)
{

    auto av_itr = avatars.find(from.value);
    check(av_itr == avatars.end(), "Account with name " + from.to_string() + " already has an avatar!");

    atomicassets::assets_t player_assets = atomicassets::get_assets(to);
    auto asset_itr = player_assets.require_find(asset_id, "No NFT with this ID exists");

    // CHECK THAT THE ASSET CORRESPONDS TO OUR COLLECTION
    check(asset_itr->collection_name == name(COLLECTION_NAME), "NFT doesn't correspond to " + COLLECTION_NAME);
    check(asset_itr->schema_name == name(AVATAR_SCHEMA_NAME), "NFT doesn't correspond to schema " + AVATAR_SCHEMA_NAME);

    auto ac_itr = accounts.find(from.value);

    if (ac_itr == accounts.end()) {

        auto config_itr = config.begin();

        vector<asset> balances;

        asset credits;
        credits.symbol = CREDITS_SYMBOL;
        credits.amount = 0;
        balances.push_back(credits);
        
        asset carbs;
        carbs.symbol = CARBS_SYMBOL;
        carbs.amount = 0;
        balances.push_back(carbs);

        asset jigowatts;
        jigowatts.symbol = JIGOWATTS_SYMBOL;
        jigowatts.amount = 0;
        balances.push_back(jigowatts);

        accounts.emplace(CONTRACTN, [&](auto& acc) {
            acc.account = from;
            acc.stamina = config_itr->init_stamina;
            acc.battery = config_itr->init_battery;
            acc.balances = balances;
            acc.unclaimed_credits = credits;
        });

        avatars.emplace(CONTRACTN, [&](auto& acc) {
            acc.account = from;
            acc.avatar_id = asset_id;
        });
    }
}

void clashdomewld::stakeTool(uint64_t asset_id, name from, name to)
{

    atomicassets::assets_t player_assets = atomicassets::get_assets(to);
    auto asset_itr = player_assets.require_find(asset_id, "No NFT with this ID exists");

    // CHECK THAT THE ASSET CORRESPONDS TO OUR COLLECTION
    check(asset_itr->collection_name == name(COLLECTION_NAME), "NFT doesn't correspond to " + COLLECTION_NAME);
    check(asset_itr->schema_name == name(TOOL_SCHEMA_NAME), "NFT doesn't correspond to schema " + TOOL_SCHEMA_NAME);

    atomicassets::schemas_t collection_schemas = atomicassets::get_schemas(name(COLLECTION_NAME));
    auto schema_itr = collection_schemas.find(name(TOOL_SCHEMA_NAME).value);

    atomicassets::templates_t collection_templates = atomicassets::get_templates(name(COLLECTION_NAME));
    auto template_itr = collection_templates.find(asset_itr->template_id);

    vector <uint8_t> immutable_serialized_data = template_itr->immutable_serialized_data;

    atomicassets::ATTRIBUTE_MAP idata = atomicdata::deserialize(immutable_serialized_data, schema_itr->format);

    string type = get<string> (idata["type"]);

    // TODO: remove this
    if (type == "Electrolyte") {
        type = "Carbs";
    }

    checkEmptySlot(type);

    tools.emplace(CONTRACTN, [&](auto& tool) {
        tool.asset_id = asset_id;
        tool.owner = from;
        tool.type = type;
        tool.template_id = asset_itr->template_id;
        tool.battery = get<uint16_t> (idata["battery"]);
        tool.max_battery = get<uint16_t> (idata["battery"]);
        tool.integrity = get<uint16_t> (idata["integrity"]);
        tool.max_integrity = get<uint16_t> (idata["integrity"]);
        tool.last_claim = 0;
    });
}

void clashdomewld::craftTool(name account, uint32_t template_id)
{

    auto ac_itr = accounts.find(account.value);
    check(ac_itr != accounts.end(), "Account with name " + account.to_string() + " doesn't exists!");

    auto tool_itr = toolconfig.find(template_id);
    check(tool_itr != toolconfig.end(), "Tool with template id " + to_string(template_id) + " doesn't exists!");

    // check all necesary materials && remove materials

    for(auto i = 0; i < tool_itr->craft.size(); i++) {

        uint64_t pos = finder(ac_itr->balances, tool_itr->craft[i].symbol);

        check(pos != -1, "Insufficient materials.");
        check(ac_itr->balances[pos].amount >= tool_itr->craft[i].amount, "Insufficient materials.");

        accounts.modify(ac_itr, CONTRACTN, [&](auto& acc) {
            acc.balances.at(pos) -= tool_itr->craft[i];
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
            name(TOOL_SCHEMA_NAME),
            template_id,
            account,
            (atomicassets::ATTRIBUTE_MAP) {},
            (atomicassets::ATTRIBUTE_MAP) {},
            (vector <asset>) {}
        )
    ).send();
}

void clashdomewld::stakeSlot(uint64_t asset_id, name from, name to, string type)
{

    atomicassets::assets_t player_assets = atomicassets::get_assets(to);
    auto asset_itr = player_assets.require_find(asset_id, "No NFT with this ID exists");

    // CHECK THAT THE ASSET CORRESPONDS TO OUR COLLECTION
    check(asset_itr->collection_name == name(COLLECTION_NAME), "NFT doesn't correspond to " + COLLECTION_NAME);
    check(asset_itr->schema_name == name(SLOT_SCHEMA_NAME), "NFT doesn't correspond to schema " + SLOT_SCHEMA_NAME);

    uint64_t slots_count = 0;

    auto slot_idx = slots.get_index<name("byowner")>();

    for ( auto slot_itr = slot_idx.begin(); slot_itr != slot_idx.end(); slot_itr++ ) {
        if (slot_itr->type == type) {
            slots_count++;
        }
    }

    check(slots_count < MAX_SLOTS - 1, "You already have stakead the maximum number of slots for " + type);

    // atomicassets::schemas_t collection_schemas = atomicassets::get_schemas(name(COLLECTION_NAME));
    // auto schema_itr = collection_schemas.find(name(SLOT_SCHEMA_NAME).value);

    // atomicassets::templates_t collection_templates = atomicassets::get_templates(name(COLLECTION_NAME));
    // auto template_itr = collection_templates.find(asset_itr->template_id);

    // vector <uint8_t> immutable_serialized_data = template_itr->immutable_serialized_data;

    // atomicassets::ATTRIBUTE_MAP idata = atomicdata::deserialize(immutable_serialized_data, schema_itr->format);

    slots.emplace(CONTRACTN, [&](auto& slot) {
        slot.asset_id = asset_id;
        slot.owner = from;
        slot.type = type;
        slot.template_id = asset_itr->template_id;
    });
}

void clashdomewld::craftSlot(name account, uint32_t template_id)
{

    auto ac_itr = accounts.find(account.value);
    check(ac_itr != accounts.end(), "Account with name " + account.to_string() + " doesn't exists!");

    auto slot_itr = slotsconfig.find(template_id);
    check(slot_itr != slotsconfig.end(), "Slot with template id " + to_string(template_id) + " doesn't exists!");

    // check all necesary materials && remove materials

    for(auto i = 0; i < slot_itr->craft.size(); i++) {

        uint64_t pos = finder(ac_itr->balances, slot_itr->craft[i].symbol);

        check(pos != -1, "Insufficient materials.");
        check(ac_itr->balances[pos].amount >= slot_itr->craft[i].amount, "Insufficient materials.");

        accounts.modify(ac_itr, CONTRACTN, [&](auto& acc) {
            acc.balances.at(pos) -= slot_itr->craft[i];
        });
    }

    // mint and send slot
    action (
        permission_level{get_self(), name("active")},
        atomicassets::ATOMICASSETS_ACCOUNT,
        name("mintasset"),
        std::make_tuple(
            get_self(),
            name(COLLECTION_NAME),
            name(SLOT_SCHEMA_NAME),
            template_id,
            account,
            (atomicassets::ATTRIBUTE_MAP) {},
            (atomicassets::ATTRIBUTE_MAP) {},
            (vector <asset>) {}
        )
    ).send();
}

void clashdomewld::unstakeSlot(
    name account,
    uint64_t asset_id
) {

    auto slot_itr = slots.find(asset_id);

    check(slot_itr != slots.end(), "Tool with id " + to_string(asset_id) + " doesn't exists!");
    check(slot_itr->owner == account, "Account " + account.to_string() + " isn't the owner of asset " + to_string(asset_id));

    checkEmptySlot(slot_itr->type);

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
            "Slot " + to_string(asset_id) + " unstake."
        )
    ).send();

    slots.erase(slot_itr);
}

void clashdomewld::checkEmptySlot(
    string type
) {

    uint64_t slots_count = 1;
    uint64_t staked_tools = 0;

    auto slot_idx = slots.get_index<name("byowner")>();

    for ( auto slot_itr = slot_idx.begin(); slot_itr != slot_idx.end(); slot_itr++ ) {
        if (slot_itr->type == type) {
            slots_count++;
        }
    }

    auto tool_idx = tools.get_index<name("byowner")>();

    for ( auto tool_itr = tool_idx.begin(); tool_itr != tool_idx.end(); tool_itr++ ) {
        if (tool_itr->type == type) {
            staked_tools++;
        }
    }

    check(staked_tools < slots_count, "You don't have empty slot for " + type + ".");
}
