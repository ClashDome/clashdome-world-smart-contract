#include <clashdomewld.hpp>

void clashdomewld::unstake(
    name account,
    uint64_t asset_id,
    string type
) {

    require_auth(account);

    if (type == TOOL_SCHEMA_NAME) {

        auto ac_itr = accounts.find(account.value);
        check(ac_itr != accounts.end(), "Account with name " + account.to_string() + " doesn't exist!");

        auto tool_itr = tools.find(asset_id);

        check(tool_itr != tools.end(), "Tool with id " + to_string(asset_id) + " doesn't exist!");
        check(tool_itr->owner == account, "Account " + account.to_string() + " isn't the owner of asset " + to_string(asset_id));

        auto tool_conf_itr = toolconfig.find(tool_itr->template_id);
        check(tool_conf_itr != toolconfig.end(), "Tool with template id " + to_string(tool_itr->template_id) + " doesn't exist!");

        uint64_t timestamp = eosio::current_time_point().sec_since_epoch();

        check(tool_itr->last_claim + tool_conf_itr->cooldown <= timestamp, "Claim not available yet. Wait until you have the tool claimable.");
        check(tool_itr->battery == tool_itr->max_battery, "You need to charge battery!");
        check(tool_itr->integrity == tool_itr->max_integrity, "You need to charge integrity!");

        if (tool_itr->type == "Carbz") {
            accounts.modify(ac_itr, CONTRACTN, [&](auto& acc) {
                acc.carbz_free_slots += 1;
            });
        } else if (tool_itr->type == "Jigowatts") {
            accounts.modify(ac_itr, CONTRACTN, [&](auto& acc) {
                acc.jigowatts_free_slots += 1;
            });
        }

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

        auto ac_itr = accounts.find(account.value);
        check(ac_itr != accounts.end(), "Account with name " + account.to_string() + " doesn't exist!");

        auto slot_itr = slots.find(asset_id);

        check(slot_itr != slots.end(), "Slot with id " + to_string(asset_id) + " doesn't exist!");
        check(slot_itr->owner == account, "Account " + account.to_string() + " isn't the owner of asset " + to_string(asset_id));

        if (slot_itr->type == "Carbz") {
            check(ac_itr->carbz_free_slots > 0, "You don't have empty slot for " + slot_itr->type + ".");

            accounts.modify(ac_itr, CONTRACTN, [&](auto& acc) {
                acc.carbz_slots -= 1;
                acc.carbz_free_slots -= 1;
            });
        } else if (slot_itr->type == "Jigowatts") {
            check(ac_itr->jigowatts_free_slots > 0, "You don't have empty slot for " + slot_itr->type + ".");

            accounts.modify(ac_itr, CONTRACTN, [&](auto& acc) {
                acc.jigowatts_slots -= 1;
                acc.jigowatts_free_slots -= 1;
            });
        }

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
    } else if (type == CITIZEN_SCHEMA_NAME) {

        auto config_itr = config.begin();

        auto ac_itr = accounts.find(account.value);
        check(ac_itr != accounts.end(), "Account with name " + account.to_string() + " doesn't exist!");

        auto citizen_itr = citizens.find(account.value);
        check(citizen_itr != citizens.end(), "Account with name " + account.to_string() + " has no citizen staked!");

        check(citizen_itr->citizen_id == asset_id, "Invalid citizen id.");
        
        auto citizen_config_itr = citizconfig.find(citizen_itr->type);

        check(ac_itr->stamina >= citizen_config_itr->max_stamina, "You need to restore your Stamina first.");
        check(ac_itr->battery >= config_itr->max_battery, "You need to recharge your Battery first.");

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
                "Citizen " + to_string(asset_id) + " unstake."
            )
        ).send();

        citizens.erase(citizen_itr);
    } else if (type == WALLET_SCHEMA_NAME) {

        auto ac_itr = accounts.find(account.value);
        check(ac_itr != accounts.end(), "Account with name " + account.to_string() + " doesn't exist!");

        auto wallet_itr = wallets.find(asset_id);

        check(wallet_itr != wallets.end(), "Wallet with id " + to_string(asset_id) + " doesn't exist!");
        check(wallet_itr->owner == account, "Account " + account.to_string() + " isn't the owner of asset " + to_string(asset_id));

        check(ac_itr->unclaimed_credits.amount == 0, "You need to claim your Credits first.");

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
                "Wallet " + to_string(asset_id) + " unstake."
            )
        ).send();

        wallets.erase(wallet_itr);
    }
}

void clashdomewld::withdraw(
    name account,
    vector<asset> quantities
) {

    require_auth(account);

    auto ac_itr = accounts.find(account.value);
    check(ac_itr != accounts.end(), "Account with name " + account.to_string() + " doesn't exist!");

    for (auto i = 0; i < quantities.size(); i++) {

        if (quantities[i].amount > 0) {
            uint64_t pos = finder(ac_itr->balances, quantities[i].symbol);

            check(pos != -1, "Invalid symbol.");

            check(ac_itr->balances.at(pos).amount >= quantities[i].amount, "Invalid amount.");

            accounts.modify(ac_itr, CONTRACTN, [&](auto& account) {
                account.balances.at(pos) -= quantities[i];
            });

            asset quantity2;
            
            quantity2.amount = quantities[i].amount;

            if (quantities[i].symbol == CREDITS_SYMBOL) {
                quantity2.symbol = LUDIO_SYMBOL;
            } else if (quantities[i].symbol == CARBZ_SYMBOL) {
                quantity2.symbol = CDCARBZ_SYMBOL;
            } else if (quantities[i].symbol == JIGOWATTS_SYMBOL) {
                quantity2.symbol = CDJIGO_SYMBOL;
            }

            action(
                permission_level{get_self(), name("active")},
                name("clashdometkn"),
                name("transfer"),
                std::make_tuple(
                    get_self(),
                    account,
                    quantity2,
                    "Withdraw " + account.to_string()
                )
            ).send();
        }
    }
}

void clashdomewld::withdrawgs(
    name account,
    vector<asset> quantities,
    vector<uint8_t> choices,
    uint64_t timestamp
) {

    require_auth(account);

    auto ac_itr = accounts.find(account.value);
    check(ac_itr != accounts.end(), "Account with name " + account.to_string() + " doesn't exist!");

    check(choices.size() == 3, "Invalid choices length.");

    for (auto i = 0; i < choices.size(); i++) {
        check(choices[i] >= 0 && choices[i] <= 2, "Invalid choices. Must be a number between 0 and 2.");
    }

    for (auto i = 0; i < quantities.size(); i++) {
        if (quantities[i].amount > 0) {
            uint64_t pos = finder(ac_itr->balances, quantities[i].symbol);
            check(pos != -1, "Invalid symbol.");
            check(ac_itr->balances.at(pos).amount >= quantities[i].amount, "Invalid amount.");
        }
    }

    auto size = transaction_size();
    char buf[size];

    auto read = read_transaction(buf, size);
    check(size == read, "read_transaction() has failed.");

    checksum256 tx_id = eosio::sha256(buf, read);

    uint64_t signing_value;

    memcpy(&signing_value, tx_id.data(), sizeof(signing_value));

    auto gs_itr = gigaswap.find(account.value);

    vector<uint8_t> opponent_choices;

    if (gs_itr == gigaswap.end()) {

        gigaswap.emplace(CONTRACTN, [&](auto& swap) {
            swap.account = account;
            swap.timestamp = timestamp;
            swap.choices = choices;
            swap.quantities = quantities;
            swap.status = PENDING;
            swap.opponent = name("");
            swap.opponent_choices = opponent_choices;
            swap.winner = name("");
        });

    } else {
        
        check(gs_itr->status == DONE, "You already have a pending Gigaswap.");

        gigaswap.modify(gs_itr, CONTRACTN, [&](auto& swap) {
            swap.timestamp = timestamp;
            swap.choices = choices;
            swap.quantities = quantities;
            swap.status = PENDING;
            swap.opponent = name("");
            swap.opponent_choices = opponent_choices;
            swap.winner = name("");
        });
    }

    action(
        permission_level{get_self(), name("active")},
        name("orng.wax"),
        name("requestrand"),
        std::make_tuple(
            account.value, //used as assoc id
            signing_value,
            get_self()
        )
    ).send();
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
    } else if (type == WALLET_SCHEMA_NAME) {
        craftWallet(account, template_id);
    }
}

void clashdomewld::repairbat(
    name account,
    uint64_t asset_id
) {

    require_auth(account);

    auto config_itr = config.begin();

    auto ac_itr = accounts.find(account.value);
    check(ac_itr != accounts.end(), "Account with name " + account.to_string() + " doesn't exist!");

    auto tool_itr = tools.find(asset_id);
    check(tool_itr != tools.end(), "Tool with id " + to_string(asset_id) + " doesn't exist!");
    
    check(tool_itr->owner == account, "Account " + account.to_string() + " isn't the owner of asset " + to_string(asset_id));

    auto tool_conf_itr = toolconfig.find(tool_itr->template_id);
    check(tool_conf_itr != toolconfig.end(), "Tool with template id " + to_string(tool_itr->template_id) + " doesn't exist!");

    uint64_t pos = finder(ac_itr->balances, JIGOWATTS_SYMBOL);
    check(pos != -1, "Invalid symbol.");

    uint16_t battery_necessary = tool_itr->max_battery - tool_itr->battery;

    check(battery_necessary > 0, "No need to repair Battery.");

    check(ac_itr->balances[pos].amount * config_itr->jigowatts_to_battery >= battery_necessary * 10000, "Insufficient Jigowatts.");

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
    check(ac_itr != accounts.end(), "Account with name " + account.to_string() + " doesn't exist!");

    auto tool_itr = tools.find(asset_id);
    check(tool_itr != tools.end(), "Tool with id " + to_string(asset_id) + " doesn't exist!");
    
    check(tool_itr->owner == account, "Account " + account.to_string() + " isn't the owner of asset " + to_string(asset_id));

    auto tool_conf_itr = toolconfig.find(tool_itr->template_id);
    check(tool_conf_itr != toolconfig.end(), "Tool with template id " + to_string(tool_itr->template_id) + " doesn't exist!");

    uint64_t pos = finder(ac_itr->balances, CREDITS_SYMBOL);
    check(pos != -1, "Invalid symbol.");

    uint16_t integrity_necessary = tool_itr->max_integrity - tool_itr->integrity;

    check(integrity_necessary > 0, "No need to repair Integrity.");
    
    check(ac_itr->balances[pos].amount * config_itr->credits_to_integrity >= integrity_necessary * 10000, "Insufficient Credits.");

    tools.modify(tool_itr, CONTRACTN, [&](auto& tool) {
        tool.integrity += integrity_necessary;
    });

    accounts.modify(ac_itr, CONTRACTN, [&](auto& acc) {
        acc.balances.at(pos).amount -= (integrity_necessary * 10000) / config_itr->credits_to_integrity;
    });
}

void clashdomewld::repairstamina(
    name account
) {

    require_auth(account);

    auto config_itr = config.begin();

    auto ac_itr = accounts.find(account.value);
    check(ac_itr != accounts.end(), "Account with name " + account.to_string() + " doesn't exist!");

    auto citizen_itr = citizens.find(account.value);
    check(citizen_itr != citizens.end(), "Account with name " + account.to_string() + " doesn't has an citizen!");

    auto citizconfig_itr = citizconfig.find(citizen_itr->type);

    uint64_t pos = finder(ac_itr->balances, CARBZ_SYMBOL);
    check(pos != -1, "Invalid symbol.");

    check(ac_itr->stamina < citizconfig_itr->max_stamina, "You already have maximum stamina.");

    uint16_t stamina = citizconfig_itr->max_stamina - ac_itr->stamina;
    
    check(ac_itr->balances[pos].amount * config_itr->carbz_to_stamina >= stamina * 10000, "Insufficient Carbz.");

    accounts.modify(ac_itr, CONTRACTN, [&](auto& acc) {
        acc.stamina += stamina;
        acc.balances.at(pos).amount -= (stamina * 10000) / config_itr->carbz_to_stamina;
    });
}

void clashdomewld::repairaccbat(
    name account
) {

    require_auth(account);

    auto config_itr = config.begin();

    auto ac_itr = accounts.find(account.value);
    check(ac_itr != accounts.end(), "Account with name " + account.to_string() + " doesn't exist!");

    uint64_t pos = finder(ac_itr->balances, JIGOWATTS_SYMBOL);
    check(pos != -1, "Invalid symbol.");

    check(ac_itr->battery < config_itr->max_battery, "You already have maximum battery.");

    uint16_t battery = config_itr->max_battery - ac_itr->battery;
    
    check(ac_itr->balances[pos].amount * config_itr->jigowatts_to_battery >= battery * 10000, "Insufficient Jigowatts.");

    accounts.modify(ac_itr, CONTRACTN, [&](auto& acc) {
        acc.battery += battery;
        acc.balances.at(pos).amount -= (battery * 10000) / config_itr->jigowatts_to_battery;
    });
}

void clashdomewld::claimtool(
    name account,
    uint64_t asset_id
) {

    require_auth(account);

    auto ac_itr = accounts.find(account.value);
    check(ac_itr != accounts.end(), "Account with name " + account.to_string() + " doesn't exist!");

    auto citizen_itr = citizens.find(account.value);
    check(citizen_itr != citizens.end(), "Account with name " + account.to_string() + " has no citizen staked!");

    auto tool_itr = tools.find(asset_id);

    check(tool_itr != tools.end(), "Tool with id " + to_string(asset_id) + " doesn't exist!");
    check(tool_itr->owner == account, "Account " + account.to_string() + " isn't the owner of asset " + to_string(asset_id));

    auto tool_conf_itr = toolconfig.find(tool_itr->template_id);
    check(tool_conf_itr != toolconfig.end(), "Tool with template id " + to_string(tool_itr->template_id) + " doesn't exist!");

    uint64_t timestamp = eosio::current_time_point().sec_since_epoch();

    check(tool_itr->last_claim + tool_conf_itr->cooldown <= timestamp, "Claim not available yet.");
    check(ac_itr->stamina - tool_conf_itr->stamina_consumed >= 0, "Insufficient stamina.");
    check(tool_itr->battery - tool_conf_itr->battery_consumed >= 0, "Insufficient battery.");
    check(tool_itr->integrity - tool_conf_itr->integrity_consumed >= 0, "Insufficient integrity.");

    uint64_t pos = finder(ac_itr->balances, tool_conf_itr->rewards[0].symbol);

    auto citizen_config_itr = citizconfig.find(citizen_itr->type);

    uint64_t claimed = (tool_conf_itr->rewards[0].amount * (100 + citizen_config_itr->claim_extra)) / 100;

    accounts.modify(ac_itr, CONTRACTN, [&](auto& acc) {
        acc.stamina -= tool_conf_itr->stamina_consumed;
        acc.balances.at(pos).amount += claimed;
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

    auto ac_itr = accounts.find(account.value);
    check(ac_itr != accounts.end(), "Account with name " + account.to_string() + " doesn't exist!");

    auto citizen_itr = citizens.find(account.value);
    check(citizen_itr != citizens.end(), "Account with name " + account.to_string() + " has no citizen staked!");

    check(ac_itr->unclaimed_credits.amount > 0, "Nothing to claim.");

    uint64_t unclaimed_credits = ac_itr->unclaimed_credits.amount;
    uint64_t pos = finder(ac_itr->balances, ac_itr->unclaimed_credits.symbol);
    vector<string> unclaimed_actions;

    auto config_itr = config.begin();

    auto wallet_idx = wallets.get_index<name("byowner")>();
    auto wallet_itr = wallet_idx.find(account.value);

    uint16_t wallet_stamina_consumed;
    uint16_t wallet_battery_consumed;

    if (wallet_itr == wallet_idx.end()) {
        wallet_stamina_consumed = config_itr->wallet_stamina_consumed;
        wallet_battery_consumed = config_itr->wallet_battery_consumed;
    } else {

        auto wallet_conf_itr = walletconfig.find(wallet_itr->template_id);

        wallet_stamina_consumed = wallet_conf_itr->stamina_consumed;
        wallet_battery_consumed = wallet_conf_itr->battery_consumed;
    }

    check(ac_itr->stamina >= wallet_stamina_consumed, "Insufficient stamina.");
    check(ac_itr->battery >= wallet_battery_consumed, "Insufficient battery.");

    accounts.modify(ac_itr, CONTRACTN, [&](auto& acc) {
        acc.balances.at(pos).amount += unclaimed_credits;
        acc.unclaimed_credits.amount = 0;
        acc.unclaimed_actions = unclaimed_actions;
        acc.stamina -= wallet_stamina_consumed;
        acc.battery -= wallet_battery_consumed;
    });
}

void clashdomewld::addcredits(
    name account,
    asset credits,
    vector<string> unclaimed_actions
) {

    require_auth(name("clashdomedll"));

    check(credits.symbol == CREDITS_SYMBOL, "Invalid token.");

    auto ac_itr = accounts.find(account.value);
    check(ac_itr != accounts.end(), "Account with name " + account.to_string() + " doesn't exist!");

    auto config_itr = config.begin();

    auto wallet_idx = wallets.get_index<name("byowner")>();
    auto wallet_itr = wallet_idx.find(account.value);

    uint64_t max_amount = config_itr->max_unclaimed_credits * 10000;

    if (wallet_itr != wallet_idx.end()) {

        auto wallet_conf_itr = walletconfig.find(wallet_itr->template_id);
        max_amount += wallet_conf_itr->extra_capacity * 10000;
    }

    // check(ac_itr->unclaimed_credits.amount < max_amount, "Your piggybank is full!");

    if (ac_itr->unclaimed_credits.amount + credits.amount > max_amount) {
        credits.amount = max_amount - ac_itr->unclaimed_credits.amount;
    }

    vector<string> new_actions = ac_itr->unclaimed_actions;

    for (auto i = 0; i < unclaimed_actions.size(); i++) {
            new_actions.push_back(unclaimed_actions.at(i));
        }

    accounts.modify(ac_itr, CONTRACTN, [&](auto& account_itr) {
        account_itr.unclaimed_credits.amount += credits.amount;
        account_itr.unclaimed_actions = new_actions;
    });
}

// TODO: REMOVE THIS FUNCTION ONCE CLASHDOMEDLL IS THE ONLY CONTRACT HANDLING DUELS
void clashdomewld::addcredits2(
    name account,
    asset credits,
    vector<string> unclaimed_actions
) {

    require_auth(name("clashdomedls"));

    check(credits.symbol == CREDITS_SYMBOL, "Invalid token.");

    auto ac_itr = accounts.find(account.value);

    if (ac_itr != accounts.end()) {
        auto config_itr = config.begin();

        auto wallet_idx = wallets.get_index<name("byowner")>();
        auto wallet_itr = wallet_idx.find(account.value);

        uint64_t max_amount = config_itr->max_unclaimed_credits * 10000;

        if (wallet_itr != wallet_idx.end()) {

            auto wallet_conf_itr = walletconfig.find(wallet_itr->template_id);
            max_amount += wallet_conf_itr->extra_capacity * 10000;
        }

        // check(ac_itr->unclaimed_credits.amount < max_amount, "Your piggybank is full!");

        if (ac_itr->unclaimed_credits.amount + credits.amount > max_amount) {
            credits.amount = max_amount - ac_itr->unclaimed_credits.amount;
        }

        vector<string> new_actions = ac_itr->unclaimed_actions;

        for (auto i = 0; i < unclaimed_actions.size(); i++) {
                new_actions.push_back(unclaimed_actions.at(i));
            }

        accounts.modify(ac_itr, CONTRACTN, [&](auto& account_itr) {
            account_itr.unclaimed_credits.amount += credits.amount;
            account_itr.unclaimed_actions = new_actions;
        });
    }
}

void clashdomewld::addmaterials(
    name account,
    vector<asset> quantities
) {

    require_auth(get_self());

    auto ac_itr = accounts.find(account.value);
    check(ac_itr != accounts.end(), "Account with name " + account.to_string() + " doesn't exist!");

    for (auto i = 0; i < quantities.size(); i++) {

        uint64_t pos = finder(ac_itr->balances, quantities[i].symbol);

        check(pos != -1, "Invalid symbol.");

        accounts.modify(ac_itr, CONTRACTN, [&](auto& account) {
            account.balances.at(pos) += quantities[i];
        });
    }
}

void clashdomewld::setconfig(
    uint16_t init_battery,
    uint16_t max_battery,
    uint32_t max_unclaimed_credits,
    uint16_t carbz_to_stamina,
    uint16_t jigowatts_to_battery,
    uint16_t credits_to_integrity,
    uint16_t wallet_stamina_consumed,
    uint16_t wallet_battery_consumed
) {

    require_auth(get_self());

    uint64_t timestamp = eosio::current_time_point().sec_since_epoch();

    auto config_itr = config.begin();
        
    if (config_itr == config.end()) {
        config.emplace(CONTRACTN, [&](auto& config_row) {
            config_row.key = 1;
            config_row.init_battery = init_battery;
            config_row.max_battery = max_battery;
            config_row.max_unclaimed_credits = max_unclaimed_credits;
            config_row.carbz_to_stamina = carbz_to_stamina;
            config_row.jigowatts_to_battery = jigowatts_to_battery;
            config_row.wallet_stamina_consumed = wallet_stamina_consumed;
            config_row.wallet_battery_consumed = wallet_battery_consumed;
        });
    } else {
        config.modify(config_itr, CONTRACTN, [&](auto& config_row) {
            config_row.init_battery = init_battery;
            config_row.max_battery = max_battery;
            config_row.max_unclaimed_credits = max_unclaimed_credits;
            config_row.carbz_to_stamina = carbz_to_stamina;
            config_row.jigowatts_to_battery = jigowatts_to_battery;
            config_row.credits_to_integrity = credits_to_integrity;
            config_row.wallet_stamina_consumed = wallet_stamina_consumed;
            config_row.wallet_battery_consumed = wallet_battery_consumed;
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
        uint16_t battery,
        uint16_t integrity,
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
            config_row.battery = battery;
            config_row.integrity = integrity;
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
            config_row.battery = battery;
            config_row.integrity = integrity;
            config_row.cooldown = cooldown;
            config_row.craft = craft;
            config_row.rewards = rewards;
        });
    }
}

void clashdomewld::setcooldown(
    uint32_t template_id,
    uint16_t cooldown
) {
    require_auth(get_self());

    auto tool_conf_itr = toolconfig.find(template_id);
    check(tool_conf_itr != toolconfig.end(), "Tool with template id " + to_string(template_id) + " doesn't exist!");

    toolconfig.modify(tool_conf_itr, CONTRACTN, [&](auto& config_row) {
        config_row.cooldown = cooldown;
    });

}

void clashdomewld::setwalletconf(
    uint32_t template_id,
    string wallet_name,
    string img,
    name schema_name,
    string rarity,
    uint32_t extra_capacity,
    uint16_t battery_consumed,
    uint16_t stamina_consumed,
    vector<asset> craft
) {
    require_auth(get_self());

    auto config_itr = walletconfig.find(template_id);

    if (config_itr == walletconfig.end()) {
        walletconfig.emplace(CONTRACTN, [&](auto& config_row) {
            config_row.template_id = template_id;
            config_row.wallet_name = wallet_name;
            config_row.img = img;
            config_row.schema_name = schema_name;
            config_row.rarity = rarity;
            config_row.extra_capacity = extra_capacity;
            config_row.battery_consumed = battery_consumed;
            config_row.stamina_consumed = stamina_consumed;
            config_row.craft = craft;
        });
    } else {
        walletconfig.modify(config_itr, CONTRACTN, [&](auto& config_row) {
            config_row.template_id = template_id;
            config_row.wallet_name = wallet_name;
            config_row.img = img;
            config_row.schema_name = schema_name;
            config_row.rarity = rarity;
            config_row.extra_capacity = extra_capacity;
            config_row.battery_consumed = battery_consumed;
            config_row.stamina_consumed = stamina_consumed;
            config_row.craft = craft;
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

void clashdomewld::setciticonfig(
    uint8_t key,   
    string type,
    uint16_t max_stamina,
    uint8_t claim_extra
) {
    require_auth(get_self());

    auto config_itr = citizconfig.find(key);

    if (config_itr == citizconfig.end()) {
        citizconfig.emplace(CONTRACTN, [&](auto& config_row) {
            config_row.key = key;
            config_row.type = type;
            config_row.max_stamina = max_stamina;
            config_row.claim_extra = claim_extra;
        });
    } else {
        citizconfig.modify(config_itr, CONTRACTN, [&](auto& config_row) {
            config_row.type = type;
            config_row.max_stamina = max_stamina;
            config_row.claim_extra = claim_extra;
        });
    }
}

void clashdomewld::erasetoolconf(
    uint32_t template_id
) {

    require_auth(get_self());

    auto config_itr = toolconfig.find(template_id);

    check(config_itr != toolconfig.end(), "Tool with template " + to_string(template_id) + " doesn't exist!");

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
    } else if (table_name == "citizens") {
        for (auto itr = citizens.begin(); itr != citizens.end();) {
            itr = citizens.erase(itr);
        }
    } else if (table_name == "toolconfig") {
        for (auto itr = toolconfig.begin(); itr != toolconfig.end();) {
            itr = toolconfig.erase(itr);
        }
    } else if (table_name == "citizconfig") {
        for (auto itr = citizconfig.begin(); itr != citizconfig.end();) {
            itr = citizconfig.erase(itr);
        }
    } else if (table_name == "slotsconfig") {
        for (auto itr = slotsconfig.begin(); itr != slotsconfig.end();) {
            itr = slotsconfig.erase(itr);
        }
    } else if (table_name == "walletconfig") {
        for (auto itr = walletconfig.begin(); itr != walletconfig.end();) {
            itr = walletconfig.erase(itr);
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
    } else if (table_name == "wallets") {
        for (auto itr = wallets.begin(); itr != wallets.end();) {
            itr = wallets.erase(itr);
        }
    } else if (table_name == "gigaswap") {
        for (auto itr = gigaswap.begin(); itr != gigaswap.end();) {
            itr = gigaswap.erase(itr);
        }
    } else if (table_name == "gigasconfig2") {
        for (auto itr = gigasconfig2.begin(); itr != gigasconfig2.end();) {
            itr = gigasconfig2.erase(itr);
        }
    }
}

void clashdomewld::setaccvalues(
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
) {
    require_auth(get_self());

    auto ac_itr = accounts.find(account.value);
    check(ac_itr != accounts.end(), "Account with name " + account.to_string() + " doesn't exist!");

    accounts.modify(ac_itr, CONTRACTN, [&](auto& acc) {
        acc.stamina = stamina;
        acc.battery = battery;
        acc.carbz_slots = carbz_slots;
        acc.jigowatts_slots = jigowatts_slots;
        acc.carbz_free_slots = carbz_free_slots;
        acc.jigowatts_free_slots = jigowatts_free_slots;
        acc.balances = balances;
        acc.unclaimed_credits = unclaimed_credits;
        acc.unclaimed_actions = unclaimed_actions;
    });
}

void clashdomewld::initgsconf()
{

    require_auth(get_self());

    vector<uint8_t> choices = {ROCK, PAPER, SCISSORS};

    for (uint64_t i = 0; i < choices.size(); i++)
    {
        for (uint64_t j = 0; j < choices.size(); j++)
        {
            for (uint64_t k = 0; k < choices.size(); k++)
            {

                gigasconfig2.emplace(CONTRACTN, [&](auto& config_row) {
                    config_row.key = i * 9 + j * 3 + k;
                    config_row.choices = {choices[i], choices[j], choices[k]};
                    config_row.accounts = {name("fitzcarraldo"), name("rapturechain")};
                });
            }
        }
    }
}

void clashdomewld::addcitizen(
    name account,   
    uint8_t type,
    uint64_t citizen_id
) {

    require_auth(get_self());

    citizens.emplace(CONTRACTN, [&](auto& acc) {
        acc.account = account;
        acc.type = type;
        acc.citizen_id = citizen_id;
    });
}

void clashdomewld::addaccount(
    name account
) {

    require_auth(get_self());

    auto config_itr = config.begin();

    vector<asset> balances;

    asset credits;
    credits.symbol = CREDITS_SYMBOL;
    credits.amount = 0;
    balances.push_back(credits);
    
    asset carbz;
    carbz.symbol = CARBZ_SYMBOL;
    carbz.amount = 0;
    balances.push_back(carbz);

    asset jigowatts;
    jigowatts.symbol = JIGOWATTS_SYMBOL;
    jigowatts.amount = 0;
    balances.push_back(jigowatts);

    uint16_t initial_free_slots_value = 1;

    accounts.emplace(CONTRACTN, [&](auto& acc) {
        acc.account = account;
        acc.stamina = 100;
        acc.battery = config_itr->init_battery;
        acc.carbz_free_slots = initial_free_slots_value;
        acc.jigowatts_free_slots = initial_free_slots_value;
        acc.carbz_slots = initial_free_slots_value;
        acc.jigowatts_slots = initial_free_slots_value;
        acc.balances = balances;
        acc.unclaimed_credits = credits;
    });
}

ACTION clashdomewld::receiverand(
    uint64_t assoc_id,
    checksum256 random_value
) {

    require_auth(name("orng.wax"));

    auto gs_itr = gigaswap.find(assoc_id);

    name account = name(assoc_id);

    auto ac_itr = accounts.find(account.value);
    check(ac_itr != accounts.end(), "Account with name " + account.to_string() + " doesn't exist!");
    check(gs_itr != gigaswap.end(), "Invalid assoc_id " + account.to_string());

    //cast the random_value to a smaller number
    uint64_t max_value = 26;
    uint64_t final_random_value = 0;

    auto byte_array = random_value.extract_as_byte_array();

    uint64_t random_int = 0;
    for (int i = 0; i < 8; i++) {
        random_int <<= 8;
        random_int |= (uint64_t)byte_array[i];
    }

    final_random_value = random_int % max_value; 

    auto rival_gs_itr = gigasconfig2.find(final_random_value);

    int8_t points = 0;

    points += GIGASWAP_MAP[gs_itr->choices[0]][rival_gs_itr->choices[0]];
    points += GIGASWAP_MAP[gs_itr->choices[1]][rival_gs_itr->choices[1]];
    points += GIGASWAP_MAP[gs_itr->choices[2]][rival_gs_itr->choices[2]];

    name winner = name("");

    vector<asset> quantities = gs_itr->quantities;

    name rival_name = rival_gs_itr->accounts[0] == account ? rival_gs_itr->accounts[1] : rival_gs_itr->accounts[0];

    if (points > 0) {

        winner = account;

        for (auto i = 0; i < quantities.size(); i++) {
            if (quantities[i].amount > 0) {
                uint64_t pos = finder(ac_itr->balances, quantities[i].symbol);
                check(pos != -1, "Invalid symbol.");
                check(ac_itr->balances.at(pos).amount >= quantities[i].amount, "Invalid amount.");

                accounts.modify(ac_itr, CONTRACTN, [&](auto& acc) {
                    acc.balances.at(pos) -= quantities[i];
                });

                asset quantity2;
            
                quantity2.amount = quantities[i].amount * 2;

                if (quantities[i].symbol == CREDITS_SYMBOL) {
                    quantity2.symbol = LUDIO_SYMBOL;
                } else if (quantities[i].symbol == CARBZ_SYMBOL) {
                    quantity2.symbol = CDCARBZ_SYMBOL;
                } else if (quantities[i].symbol == JIGOWATTS_SYMBOL) {
                    quantity2.symbol = CDJIGO_SYMBOL;
                }

                action(
                    permission_level{get_self(), name("active")},
                    name("clashdometkn"),
                    name("transfer"),
                    std::make_tuple(
                        get_self(),
                        account,
                        quantity2,
                        "Gigaswap " + account.to_string()
                    )
                ).send();
            }
        }

    } else if (points < 0) {

        winner = rival_name;

        for (auto i = 0; i < quantities.size(); i++) {
            if (quantities[i].amount > 0) {
                uint64_t pos = finder(ac_itr->balances, quantities[i].symbol);
                check(pos != -1, "Invalid symbol.");
                check(ac_itr->balances.at(pos).amount >= quantities[i].amount, "Invalid amount.");

                accounts.modify(ac_itr, CONTRACTN, [&](auto& acc) {
                    acc.balances.at(pos) -= quantities[i];
                });
            }
        }
    }

    gigaswap.modify(gs_itr, CONTRACTN, [&](auto& swap) {
        swap.status = DONE;
        swap.opponent = rival_name;
        swap.opponent_choices = rival_gs_itr->choices;
        swap.winner = winner;
    });

    uint64_t initial_value = 0;
    bool found = false;

    auto player_gs_itr = gigasconfig2.find(initial_value);

    while (player_gs_itr != gigasconfig2.end() && !found) {

        if (player_gs_itr->choices[0] == gs_itr->choices[0] && player_gs_itr->choices[1] == gs_itr->choices[1] && player_gs_itr->choices[2] == gs_itr->choices[2]) {
            found = true;
            gigasconfig2.modify(player_gs_itr, CONTRACTN, [&](auto& swap) {

                if (swap.accounts[0] != account && swap.accounts[1] != account) {
                    swap.accounts[0] = swap.accounts[1];
                    swap.accounts[1] = account;
                }
            });
        } else {
            player_gs_itr++;
        }
    }

    // action(
    //     permission_level{get_self(), name("active")},
    //     get_self(),
    //     name("loggigaswap"),
    //     std::make_tuple(
    //         name(assoc_id),
    //         gs_itr->choices,
    //         rival_gs_itr->account,
    //         rival_gs_itr->choices,
    //         final_random_value,
    //         points,
    //         winner
    //     )
    // ).send();
}

void clashdomewld::loggigaswap(
    name acount,
    vector<uint8_t> player_choices,
    name rival,
    vector<uint8_t> rival_choices,
    uint64_t random_value,
    int8_t points,
    name winner
) {
    require_auth(get_self());
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

    check(asset_ids.size() == 1, "Only one NFT can be sent at a time.");

    if (memo == "stake tool") {

        stakeTool(asset_ids[0], from, to);

    } else if (memo.find("stake citizen") != string::npos) {

        stakeAvatar(asset_ids[0], from, to, memo);

    } else if (memo.find("stake slots") != string::npos) {

        if (memo.find("Carbz") != string::npos) {
            stakeSlot(asset_ids[0], from, to, "Carbz");
        } else if (memo.find("Jigowatts") != string::npos) {
            stakeSlot(asset_ids[0], from, to, "Jigowatts");
        } else {
            check(memo == "transfer", "Invalid memo slot type.");
        }
    } else if (memo == "stake wallet") {
        stakeWallet(asset_ids[0], from, to);
    } else if (memo == "get tokens") {
        getTokens(asset_ids[0], from, to);
    } else {
        check(memo == "transfer", "Invalid memo.");
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
        check(ac_itr != accounts.end(), "Stake a citizen first!!");

        asset quantity2;
        quantity2.amount = quantity.amount;

        if (quantity.symbol == LUDIO_SYMBOL) {

            uint64_t pos = finder(ac_itr->balances, CREDITS_SYMBOL);

            check(pos != -1, "Invalid symbol.");

            quantity2.symbol = CREDITS_SYMBOL;

            accounts.modify(ac_itr, CONTRACTN, [&](auto& account) {
                account.balances.at(pos) += quantity2;
            });
        } else if (quantity.symbol == CDCARBZ_SYMBOL) {

            uint64_t pos = finder(ac_itr->balances, CARBZ_SYMBOL);

            check(pos != -1, "Invalid symbol.");

            quantity2.symbol = CARBZ_SYMBOL;

            accounts.modify(ac_itr, CONTRACTN, [&](auto& account) {
                account.balances.at(pos) += quantity2;
            });
        } else if (quantity.symbol == CDJIGO_SYMBOL) {

            uint64_t pos = finder(ac_itr->balances, JIGOWATTS_SYMBOL);

            check(pos != -1, "Invalid symbol.");

            quantity2.symbol = JIGOWATTS_SYMBOL;

            accounts.modify(ac_itr, CONTRACTN, [&](auto& account) {
                account.balances.at(pos) += quantity2;
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

void clashdomewld::stakeAvatar(uint64_t asset_id, name from, name to, string memo)
{

    // const size_t fb = memo.find(":");
    // string d1 = memo.substr(0, fb);
    // string d2 = memo.substr(fb + 1);

    // checkEarlyAccess(from, stoull(d2));

    auto citizen_itr = citizens.find(from.value);
    check(citizen_itr == citizens.end(), "Account with name " + from.to_string() + " already has an citizen!");

    atomicassets::assets_t player_assets = atomicassets::get_assets(to);
    auto asset_itr = player_assets.require_find(asset_id, "No NFT with this ID exists");

    // CHECK THAT THE ASSET CORRESPONDS TO OUR COLLECTION
    check(asset_itr->collection_name == name(COLLECTION_NAME), "NFT doesn't correspond to " + COLLECTION_NAME);
    check(asset_itr->schema_name == name(CITIZEN_SCHEMA_NAME), "NFT doesn't correspond to schema " + CITIZEN_SCHEMA_NAME);

    atomicassets::schemas_t collection_schemas = atomicassets::get_schemas(name(COLLECTION_NAME));
    auto schema_itr = collection_schemas.find(name(CITIZEN_SCHEMA_NAME).value);

    atomicassets::templates_t collection_templates = atomicassets::get_templates(name(COLLECTION_NAME));
    auto template_itr = collection_templates.find(asset_itr->template_id);

    vector <uint8_t> immutable_serialized_data = template_itr->immutable_serialized_data;

    atomicassets::ATTRIBUTE_MAP idata = atomicdata::deserialize(immutable_serialized_data, schema_itr->format);

    string rarity = get<string> (idata["rarity"]);

    uint8_t type = 0;

    if (rarity == "Pleb") {
        type = 0;
    } else if (rarity == "UberNorm") {
        type = 1;
    } else if (rarity == "Hi-Clone") {
        type = 2;
    }

    citizens.emplace(CONTRACTN, [&](auto& acc) {
        acc.account = from;
        acc.type = type;
        acc.citizen_id = asset_id;
    });

    auto ac_itr = accounts.find(from.value);
    auto citizen_config_itr = citizconfig.find(type);

    if (ac_itr == accounts.end()) {

        auto config_itr = config.begin();

        vector<asset> balances;

        asset credits;
        credits.symbol = CREDITS_SYMBOL;
        credits.amount = 0;
        balances.push_back(credits);
        
        asset carbz;
        carbz.symbol = CARBZ_SYMBOL;
        carbz.amount = 0;
        balances.push_back(carbz);

        asset jigowatts;
        jigowatts.symbol = JIGOWATTS_SYMBOL;
        jigowatts.amount = 0;
        balances.push_back(jigowatts);

        uint16_t initial_free_slots_value = 1;

        accounts.emplace(CONTRACTN, [&](auto& acc) {
            acc.account = from;
            acc.stamina = citizen_config_itr->max_stamina;
            acc.battery = config_itr->init_battery;
            acc.carbz_free_slots = initial_free_slots_value;
            acc.jigowatts_free_slots = initial_free_slots_value;
            acc.carbz_slots = initial_free_slots_value;
            acc.jigowatts_slots = initial_free_slots_value;
            acc.balances = balances;
            acc.unclaimed_credits = credits;
        });
    } else {

        accounts.modify(ac_itr, CONTRACTN, [&](auto& acc) {
            acc.stamina = citizen_config_itr->max_stamina;
        });
    }
}

void clashdomewld::stakeTool(uint64_t asset_id, name from, name to)
{

    auto ac_itr = accounts.find(from.value);
    check(ac_itr != accounts.end(), "Stake a citizen first!!");

    auto citizen_itr = citizens.find(from.value);
    check(citizen_itr != citizens.end(), "Stake a citizen first!");

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

    if (type == "Carbz") {
        check(ac_itr->carbz_free_slots > 0, "You don't have empty slot for " + type + ".");

        accounts.modify(ac_itr, CONTRACTN, [&](auto& acc) {
            acc.carbz_free_slots -= 1;
        });

    } else if (type == "Jigowatts") {
        check(ac_itr->jigowatts_free_slots > 0, "You don't have empty slot for " + type + ".");

        accounts.modify(ac_itr, CONTRACTN, [&](auto& acc) {
            acc.jigowatts_free_slots -= 1;
        });
    }

    auto tool_itr = toolconfig.find(asset_itr->template_id);
    check(tool_itr != toolconfig.end(), "Tool with template id " + to_string(asset_itr->template_id) + " doesn't exist!");

    tools.emplace(CONTRACTN, [&](auto& tool) {
        tool.asset_id = asset_id;
        tool.owner = from;
        tool.type = type;
        tool.template_id = asset_itr->template_id;
        tool.battery = tool_itr->battery;
        tool.max_battery = tool_itr->battery;
        tool.integrity = tool_itr->integrity;
        tool.max_integrity = tool_itr->integrity;
        tool.last_claim = 0;
    });
}

void clashdomewld::craftTool(name account, uint32_t template_id)
{

    auto ac_itr = accounts.find(account.value);
    check(ac_itr != accounts.end(), "Account with name " + account.to_string() + " doesn't exist!");

    auto citizen_itr = citizens.find(account.value);
    check(citizen_itr != citizens.end(), "Account with name " + account.to_string() + " has no citizen staked!");

    auto tool_itr = toolconfig.find(template_id);
    check(tool_itr != toolconfig.end(), "Tool with template id " + to_string(template_id) + " doesn't exist!");

    // check all necesary materials && remove materials

    for(auto i = 0; i < tool_itr->craft.size(); i++) {

        uint64_t pos = finder(ac_itr->balances, tool_itr->craft[i].symbol);

        check(pos != -1, "Insufficient materials.");
        check(ac_itr->balances[pos].amount >= tool_itr->craft[i].amount, "Insufficient materials.");

        accounts.modify(ac_itr, CONTRACTN, [&](auto& acc) {
            acc.balances.at(pos) -= tool_itr->craft[i];
        });

        burnTokens(tool_itr->craft[i], "tool with template_id " + to_string(template_id) + ".");
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

    auto ac_itr = accounts.find(from.value);
    check(ac_itr != accounts.end(), "Stake a citizen first!!");

    auto citizen_itr = citizens.find(from.value);
    check(citizen_itr != citizens.end(), "Stake a citizen first!");

    atomicassets::assets_t player_assets = atomicassets::get_assets(to);
    auto asset_itr = player_assets.require_find(asset_id, "No NFT with this ID exists");

    // CHECK THAT THE ASSET CORRESPONDS TO OUR COLLECTION
    check(asset_itr->collection_name == name(COLLECTION_NAME), "NFT doesn't correspond to " + COLLECTION_NAME);
    check(asset_itr->schema_name == name(SLOT_SCHEMA_NAME), "NFT doesn't correspond to schema " + SLOT_SCHEMA_NAME);

    if (type == "Carbz") {
        check(ac_itr->carbz_slots < MAX_SLOTS, "You already have stakead the maximum number of slots for " + type);

        accounts.modify(ac_itr, CONTRACTN, [&](auto& acc) {
            acc.carbz_free_slots += 1;
            acc.carbz_slots += 1;
        });

    } else if (type == "Jigowatts") {
        check(ac_itr->jigowatts_slots < MAX_SLOTS, "You already have stakead the maximum number of slots for " + type);

        accounts.modify(ac_itr, CONTRACTN, [&](auto& acc) {
            acc.jigowatts_free_slots += 1;
            acc.jigowatts_slots += 1;
        });
    }

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
    check(ac_itr != accounts.end(), "Account with name " + account.to_string() + " doesn't exist!");

    auto citizen_itr = citizens.find(account.value);
    check(citizen_itr != citizens.end(), "Account with name " + account.to_string() + " has no citizen staked!");

    auto slot_itr = slotsconfig.find(template_id);
    check(slot_itr != slotsconfig.end(), "Slot with template id " + to_string(template_id) + " doesn't exist!");

    // check all necesary materials && remove materials

    for(auto i = 0; i < slot_itr->craft.size(); i++) {

        uint64_t pos = finder(ac_itr->balances, slot_itr->craft[i].symbol);

        check(pos != -1, "Insufficient materials.");
        check(ac_itr->balances[pos].amount >= slot_itr->craft[i].amount, "Insufficient materials.");

        accounts.modify(ac_itr, CONTRACTN, [&](auto& acc) {
            acc.balances.at(pos) -= slot_itr->craft[i];
        });

        burnTokens(slot_itr->craft[i], "slot with template_id " + to_string(template_id) + ".");
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

void clashdomewld::stakeWallet(uint64_t asset_id, name from, name to)
{

    auto ac_itr = accounts.find(from.value);
    check(ac_itr != accounts.end(), "Stake a citizen first!!");

    auto citizen_itr = citizens.find(from.value);
    check(citizen_itr != citizens.end(), "Stake a citizen first!");

    atomicassets::assets_t player_assets = atomicassets::get_assets(to);
    auto asset_itr = player_assets.require_find(asset_id, "No NFT with this ID exists");

    // CHECK THAT THE ASSET CORRESPONDS TO OUR COLLECTION
    check(asset_itr->collection_name == name(COLLECTION_NAME), "NFT doesn't correspond to " + COLLECTION_NAME);
    check(asset_itr->schema_name == name(WALLET_SCHEMA_NAME), "NFT doesn't correspond to schema " + WALLET_SCHEMA_NAME);

    auto wallet_idx = wallets.get_index<name("byowner")>();
    auto wallet_itr = wallet_idx.find(from.value);

    check(wallet_itr == wallet_idx.end(), "You can only have one wallet at a time.");

    wallets.emplace(CONTRACTN, [&](auto& wallet) {
        wallet.asset_id = asset_id;
        wallet.owner = from;
        wallet.template_id = asset_itr->template_id;
    });
}

void clashdomewld::craftWallet(name account, uint32_t template_id)
{

    auto ac_itr = accounts.find(account.value);
    check(ac_itr != accounts.end(), "Account with name " + account.to_string() + " doesn't exist!");

    auto citizen_itr = citizens.find(account.value);
    check(citizen_itr != citizens.end(), "Account with name " + account.to_string() + " has no citizen staked!");

    auto wallet_itr = walletconfig.find(template_id);
    check(wallet_itr != walletconfig.end(), "Wallet with template id " + to_string(template_id) + " doesn't exist!");

    // check all necesary materials && remove materials

    for(auto i = 0; i < wallet_itr->craft.size(); i++) {

        uint64_t pos = finder(ac_itr->balances, wallet_itr->craft[i].symbol);

        check(pos != -1, "Insufficient materials.");
        check(ac_itr->balances[pos].amount >= wallet_itr->craft[i].amount, "Insufficient materials.");

        accounts.modify(ac_itr, CONTRACTN, [&](auto& acc) {
            acc.balances.at(pos) -= wallet_itr->craft[i];
        });

        burnTokens(wallet_itr->craft[i], "wallet with template_id " + to_string(template_id) + ".");
    }

    // mint and send slot
    action (
        permission_level{get_self(), name("active")},
        atomicassets::ATOMICASSETS_ACCOUNT,
        name("mintasset"),
        std::make_tuple(
            get_self(),
            name(COLLECTION_NAME),
            name(WALLET_SCHEMA_NAME),
            template_id,
            account,
            (atomicassets::ATTRIBUTE_MAP) {},
            (atomicassets::ATTRIBUTE_MAP) {},
            (vector <asset>) {}
        )
    ).send();
}

void clashdomewld::getTokens(uint64_t asset_id, name from, name to)
{

    auto ac_itr = accounts.find(from.value);
    check(ac_itr != accounts.end(), "Stake a citizen first!!");

    auto citizen_itr = citizens.find(from.value);
    check(citizen_itr != citizens.end(), "Stake a citizen first!");

    atomicassets::assets_t player_assets = atomicassets::get_assets(to);
    auto asset_itr = player_assets.require_find(asset_id, "No NFT with this ID exists");

    // CHECK THAT THE ASSET CORRESPONDS TO OUR COLLECTION
    check(asset_itr->collection_name == name(COLLECTION_NAME), "NFT doesn't correspond to " + COLLECTION_NAME);
    check(asset_itr->schema_name == name(PACKS_SCHEMA_NAME), "NFT doesn't correspond to schema " + PACKS_SCHEMA_NAME);
    check(asset_itr->template_id == PACKS_TEMPLATE_ID, "NFT doesn't correspond to template id " + to_string(PACKS_TEMPLATE_ID));

    action(
        permission_level{get_self(), name("active")},
        atomicassets::ATOMICASSETS_ACCOUNT,
        name("burnasset"),
        std::make_tuple(
            get_self(),
            asset_id
        )
    ).send();

    uint64_t posJigo = finder(ac_itr->balances, JIGOWATTS_SYMBOL);
    check(posJigo != -1, "Invalid material.");

    uint64_t posCarbz = finder(ac_itr->balances, CARBZ_SYMBOL);
    check(posCarbz != -1, "Invalid material.");

    accounts.modify(ac_itr, CONTRACTN, [&](auto& account) {
        account.balances.at(posJigo).amount += PACK_JIGO_REWARD;
        account.balances.at(posCarbz).amount += PACK_CARBZ_REWARD;
    });
}

void clashdomewld::burnTokens(asset tokens, string memo_extra)
{
    asset credits;
    credits.symbol = LUDIO_SYMBOL;
    credits.amount = (tokens.amount * CRAFT_BURN_PERCENT) / 100;

    if (tokens.symbol == JIGOWATTS_SYMBOL) {
        credits.symbol = CDJIGO_SYMBOL;
    } else if (tokens.symbol == CREDITS_SYMBOL) {
        credits.symbol = LUDIO_SYMBOL;
    } else if (tokens.symbol == CARBZ_SYMBOL) {
        credits.symbol = CDCARBZ_SYMBOL;
    }

    action (
        permission_level{get_self(), name("active")},
        name("clashdometkn"),
        name("burn"),
        std::make_tuple(
            credits,
            "Crafting " + memo_extra
        )
    ).send();
}

void clashdomewld::checkEarlyAccess(name account, uint64_t early_access) {

    atomicassets::assets_t assets = atomicassets::get_assets(account);

    auto assets_itr = assets.find(early_access);

    check(assets_itr != assets.end(), "Account " + account.to_string() + " doesn't have the required NFT to play!");
    check(assets_itr->template_id == EARLY_ACCESS_TEMPLATE_ID, "The id provided does not correspond to the required NFT to play!");
}
