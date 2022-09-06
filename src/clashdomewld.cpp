#include <clashdomewld.hpp>

void clashdomewld::staketrial(
    name account,
    uint64_t asset_id
) {

    require_auth(account);

    // si ya se ha tenido una cuenta antes no se puede utilizar un trial
    auto ac_itr = accounts.find(account.value);
    check(ac_itr == accounts.end(), "Account with name " + account.to_string() + " already exists!");

    atomicassets::assets_t player_assets = atomicassets::get_assets(account);
    auto asset_itr = player_assets.require_find(asset_id, "No NFT with this ID exists");

    // CHECK THAT THE ASSET CORRESPONDS TO OUR COLLECTION / SCHEMA AND TEMPLATE
    check(asset_itr->collection_name == name(COLLECTION_NAME), "NFT doesn't correspond to " + COLLECTION_NAME);
    check(asset_itr->schema_name == name(CITIZEN_SCHEMA_NAME), "NFT doesn't correspond to schema " + SLOT_SCHEMA_NAME);
    check(asset_itr->template_id == TRIAL_TEMPLATE_ID, "NFT doesn't correspond to template id " + to_string(TRIAL_TEMPLATE_ID));

    atomicassets::schemas_t collection_schemas = atomicassets::get_schemas(name(COLLECTION_NAME));
    auto schema_itr = collection_schemas.find(name(CITIZEN_SCHEMA_NAME).value);

    vector <uint8_t> mutable_serialized_data = asset_itr->mutable_serialized_data;
    atomicassets::ATTRIBUTE_MAP mdata = atomicdata::deserialize(mutable_serialized_data, schema_itr->format);

    if (mdata.find("affiliate") != mdata.end()) {

        string partner = get<string> (mdata["affiliate"]);

        auto rf_itr = partners.find(account.value);

        if (rf_itr == partners.end()) {
            partners.emplace(CONTRACTN, [&](auto& referral) {
                referral.account = account;
                referral.partner = name(partner);
            });
        } else {
            partners.modify(rf_itr, CONTRACTN, [&](auto& referral) {
                referral.partner = name(partner);
            });
        }
    }

    auto tr_itr = trials.find(account.value);

    if (tr_itr == trials.end()) {
        asset credits;
        credits.symbol = CREDITS_SYMBOL;
        credits.amount = 0;

        trials.emplace(CONTRACTN, [&](auto& trial) {
            trial.account = account;
            trial.asset_id = asset_id;
            trial.credits = credits;
            trial.staked = true;
            trial.full = false;
        });
    } else {
        trials.modify(tr_itr, CONTRACTN, [&](auto& trial) {
            trial.staked = true;
            trial.asset_id = asset_id;
        });
    }
}

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

        auto citizen_itr = citiz.find(account.value);
        check(citizen_itr != citiz.end(), "Account with name " + account.to_string() + " has no citizen staked!");

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

        citiz.erase(citizen_itr);
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
    } else if (type == DECORATION_SCHEMA_NAME) {

        auto ac_itr = accounts.find(account.value);
        check(ac_itr != accounts.end(), "Account with name " + account.to_string() + " doesn't exist!");

        auto decoration_itr = decorations.find(asset_id);

        check(decoration_itr != decorations.end(), "Decoration with id " + to_string(asset_id) + " doesn't exist!");
        check(decoration_itr->owner == account, "Account " + account.to_string() + " isn't the owner of asset " + to_string(asset_id));

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
                "Decoration " + to_string(asset_id) + " unstake."
            )
        ).send();

        decorations.erase(decoration_itr);
    }
}

// TODO: remove this after merge
void clashdomewld::withdraw(
    name account,
    vector<asset> quantities
) {

    require_auth(name("packsopenerx"));

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

void clashdomewld::editsocial(
    name account,
    string memo
) {

    require_auth(account);

    auto ac_itr = social.find(account.value);

    check(ac_itr != social.end(), "You need to pay before smartass!");

    parseSocialsMemo(account, memo);
}

void clashdomewld::claimtool(
    name account,
    uint64_t asset_id
) {

    require_auth(account);

    auto ac_itr = accounts.find(account.value);
    check(ac_itr != accounts.end(), "Account with name " + account.to_string() + " doesn't exist!");

    auto citizen_itr = citiz.find(account.value);
    check(citizen_itr != citiz.end(), "Account with name " + account.to_string() + " has no citizen staked!");

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

    auto citizen_config_itr = citizconfig.find(citizen_itr->type);

    uint64_t claimed = (tool_conf_itr->rewards[0].amount * (100 + citizen_config_itr->claim_extra)) / 100;

    accounts.modify(ac_itr, CONTRACTN, [&](auto& acc) {
        acc.stamina -= tool_conf_itr->stamina_consumed;
    });

    tools.modify(tool_itr, CONTRACTN, [&](auto& tool) {
        tool.battery -= tool_conf_itr->battery_consumed;
        tool.integrity -= tool_conf_itr->integrity_consumed;
        tool.last_claim = timestamp;
    });

    asset quantity;
    quantity.amount = claimed;

    if (tool_conf_itr->rewards[0].symbol == CARBZ_SYMBOL) {
        quantity.symbol = CDCARBZ_SYMBOL;
    } else if (tool_conf_itr->rewards[0].symbol == JIGOWATTS_SYMBOL) {
        quantity.symbol = CDJIGO_SYMBOL;
    }

    action(
        permission_level{get_self(), name("active")},
        name("clashdometkn"),
        name("transfer"),
        std::make_tuple(
            get_self(),
            account,
            quantity,
            "Withdraw " + account.to_string()
        )
    ).send();


    //update daily token stats
    asset update_stats_asset;
    update_stats_asset.amount=claimed;
    update_stats_asset.symbol=tool_conf_itr->rewards[0].symbol;
    updateDailyStats(update_stats_asset,1);
    
}

void clashdomewld::claim(
    name account
) {

    require_auth(account);

    auto ac_itr = accounts.find(account.value);
    check(ac_itr != accounts.end(), "Account with name " + account.to_string() + " doesn't exist!");

    auto citizen_itr = citiz.find(account.value);
    check(citizen_itr != citiz.end(), "Account with name " + account.to_string() + " has no citizen staked!");

    check(ac_itr->unclaimed_credits.amount > 0, "Nothing to claim.");

    uint64_t unclaimed_credits = ac_itr->unclaimed_credits.amount;
    vector<string> unclaimed_actions;

    auto config_itr = config.begin();

    auto wallet_idx = wallets.get_index<name("byowner")>();
    auto wallet_itr = wallet_idx.find(account.value);

    uint16_t wallet_stamina_consumed;
    uint16_t wallet_battery_consumed;

    uint64_t max_claimable_credits = config_itr->max_unclaimed_credits * 10000;

    if (wallet_itr == wallet_idx.end()) {
        wallet_stamina_consumed = config_itr->wallet_stamina_consumed;
        wallet_battery_consumed = config_itr->wallet_battery_consumed;
    } else {

        auto wallet_conf_itr = walletconfig.find(wallet_itr->template_id);

        wallet_stamina_consumed = wallet_conf_itr->stamina_consumed;
        wallet_battery_consumed = wallet_conf_itr->battery_consumed;

        max_claimable_credits += wallet_conf_itr->extra_capacity * 10000;
    }

    check(ac_itr->stamina >= wallet_stamina_consumed, "Insufficient stamina.");
    check(ac_itr->battery >= wallet_battery_consumed, "Insufficient battery.");

    uint64_t claimable_credits = (unclaimed_credits > max_claimable_credits) ? max_claimable_credits : unclaimed_credits;
    uint64_t remaining_credits = unclaimed_credits - claimable_credits;

    accounts.modify(ac_itr, CONTRACTN, [&](auto& acc) {
        acc.unclaimed_credits.amount = remaining_credits;
        acc.unclaimed_actions = unclaimed_actions;
        acc.stamina -= wallet_stamina_consumed;
        acc.battery -= wallet_battery_consumed;
    });

    asset quantity;
    
    quantity.symbol = LUDIO_SYMBOL;
    quantity.amount = claimable_credits;

    action(
        permission_level{get_self(), name("active")},
        name("clashdometkn"),
        name("transfer"),
        std::make_tuple(
            get_self(),
            account,
            quantity,
            "Withdraw " + account.to_string()
        )
    ).send();
    
    //update daily token stats
    asset update_stats_asset;
    update_stats_asset.amount=claimable_credits;
    update_stats_asset.symbol=CREDITS_SYMBOL;
    updateDailyStats(update_stats_asset,1);
}

void clashdomewld::claimtrial(
    name account,
    name affiliate
) {

    // TODO: cambiar esto al tener el programa de afiliados
    require_auth(get_self());
    // require_auth(account);

    auto af_itr = affiliates.find(affiliate.value);
    check(af_itr != affiliates.end(), "Affiliated " + affiliate.to_string() + " doesn't exist!");
    check(af_itr->available_trials > 0, "Affiliated " + affiliate.to_string() + " has no trials availables!");

    auto ac_itr = accounts.find(account.value);
    check(ac_itr == accounts.end(), "Account with name " + account.to_string() + " already has a citizen!");

    auto tr_itr = trials.find(account.value);
    check(tr_itr == trials.end(), "Account with name " + account.to_string() + " already has a trial!");

    asset credits;
    credits.symbol = CREDITS_SYMBOL;
    credits.amount = 0;

    trials.emplace(CONTRACTN, [&](auto& trial) {
        trial.account = account;
        trial.credits = credits;
        trial.staked = false;
        trial.full = false;
    });

    affiliates.modify(af_itr, CONTRACTN, [&](auto& affiliate) {
        affiliate.available_trials--;
    });

    atomicassets::ATTRIBUTE_MAP mdata = {};
    mdata["affiliate"] = affiliate.to_string();

    // mint and send trial
    action (
        permission_level{get_self(), name("active")},
        atomicassets::ATOMICASSETS_ACCOUNT,
        name("mintasset"),
        std::make_tuple(
            get_self(),
            name(COLLECTION_NAME),
            name(CITIZEN_SCHEMA_NAME),
            TRIAL_TEMPLATE_ID,
            account,
            (atomicassets::ATTRIBUTE_MAP) {},
            mdata,
            (vector <asset>) {}
        )
    ).send();
}

void clashdomewld::claimhalltr(
    name account,
    uint64_t asset_id
) {
    require_auth(account);

    name affiliate = name("clashdomewld");

    auto af_itr = affiliates.find(affiliate.value);
    check(af_itr != affiliates.end(), "Affiliated " + affiliate.to_string() + " doesn't exist!");
    check(af_itr->available_trials > 0, "Affiliated " + affiliate.to_string() + " has no trials availables!");

    auto ac_itr = accounts.find(account.value);
    check(ac_itr == accounts.end(), "Account with name " + account.to_string() + " already has a citizen!");

    auto tr_itr = trials.find(account.value);
    check(tr_itr == trials.end(), "Account with name " + account.to_string() + " already has a trial!");

    atomicassets::assets_t player_assets = atomicassets::get_assets(account);
    auto asset_itr = player_assets.require_find(asset_id, "No NFT with this ID exists");

    // CHECK THAT THE ASSET CORRESPONDS TO OUR COLLECTION / SCHEMA AND TEMPLATE
    check(asset_itr->collection_name == name(COLLECTION_NAME), "NFT doesn't correspond to " + COLLECTION_NAME);
    check(asset_itr->schema_name == name(HALLS_SCHEMA_NAME), "NFT doesn't correspond to schema " + HALLS_SCHEMA_NAME);

    asset credits;
    credits.symbol = CREDITS_SYMBOL;
    credits.amount = 0;

    trials.emplace(CONTRACTN, [&](auto& trial) {
        trial.account = account;
        trial.credits = credits;
        trial.staked = false;
        trial.full = false;
    });

    affiliates.modify(af_itr, CONTRACTN, [&](auto& affiliate) {
        affiliate.available_trials--;
    });

    atomicassets::ATTRIBUTE_MAP mdata = {};
    mdata["affiliate"] = affiliate.to_string();

    // mint and send trial
    action (
        permission_level{get_self(), name("active")},
        atomicassets::ATOMICASSETS_ACCOUNT,
        name("mintasset"),
        std::make_tuple(
            get_self(),
            name(COLLECTION_NAME),
            name(CITIZEN_SCHEMA_NAME),
            TRIAL_TEMPLATE_ID,
            account,
            (atomicassets::ATTRIBUTE_MAP) {},
            mdata,
            (vector <asset>) {}
        )
    ).send();

}

void clashdomewld::addcredits(
    name account,
    asset credits,
    vector<string> unclaimed_actions,
    const string&  memo 
) {

    require_auth(name("clashdomedll"));

    check(credits.symbol == CREDITS_SYMBOL, "Invalid token.");

    auto ac_itr = accounts.find(account.value);

    if (ac_itr != accounts.end()) {


        vector<string> new_actions = ac_itr->unclaimed_actions;

        for (auto i = 0; i < unclaimed_actions.size(); i++) {
            new_actions.push_back(unclaimed_actions.at(i));
        }

        auto config_itr = config.begin();

        auto wallet_idx = wallets.get_index<name("byowner")>();
        auto wallet_itr = wallet_idx.find(account.value);

        uint64_t max_amount = config_itr->max_unclaimed_credits * 10000;

        if (wallet_itr == wallet_idx.end()) {
            if (ac_itr->unclaimed_credits.amount + credits.amount > max_amount) {
                accounts.modify(ac_itr, CONTRACTN, [&](auto& account_itr) {
                    account_itr.unclaimed_credits.amount = max_amount;
                    account_itr.unclaimed_actions = new_actions;
                });
            } else {
                accounts.modify(ac_itr, CONTRACTN, [&](auto& account_itr) {
                    account_itr.unclaimed_credits.amount += credits.amount;
                    account_itr.unclaimed_actions = new_actions;
                });
            }
        } else {
            accounts.modify(ac_itr, CONTRACTN, [&](auto& account_itr) {
                account_itr.unclaimed_credits.amount += credits.amount;
                account_itr.unclaimed_actions = new_actions;
            });
        }
    } else {
        auto tr_itr = trials.find(account.value);
        check(tr_itr != trials.end(), "Account with name " + account.to_string() + " doesn't exist!");

        vector<string> new_actions = ac_itr->unclaimed_actions;

        for (auto i = 0; i < unclaimed_actions.size(); i++) {
            new_actions.push_back(unclaimed_actions.at(i));
        }

        auto config_itr = config.begin();

        auto wallet_idx = wallets.get_index<name("byowner")>();
        auto wallet_itr = wallet_idx.find(account.value);

        if (tr_itr->credits.amount + credits.amount > TRIAL_MAX_UNCLAIMED) {
            trials.modify(tr_itr, CONTRACTN, [&](auto& account_itr) {
                account_itr.credits.amount = TRIAL_MAX_UNCLAIMED;
                account_itr.unclaimed_actions = new_actions;
                account_itr.full = true;
            });
        } else {
            trials.modify(tr_itr, CONTRACTN, [&](auto& account_itr) {
                account_itr.credits.amount += credits.amount;
                account_itr.unclaimed_actions = new_actions;
            });
        }
    }   
}

void clashdomewld::addaffiliate(
    name account,
    uint8_t commission,
    uint16_t available_trials
) {

    require_auth(get_self());

    auto ac_itr = affiliates.find(account.value);

    if (ac_itr != affiliates.end()) {
        affiliates.modify(ac_itr, CONTRACTN, [&](auto& account_itr) {
            account_itr.commission = commission;
            account_itr.available_trials = available_trials;
        });
    } else {
        affiliates.emplace(CONTRACTN, [&](auto& account_itr) {
            account_itr.account = account;
            account_itr.commission = commission;
            account_itr.available_trials = available_trials;
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

void clashdomewld::setshopitem(
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
) {
    require_auth(get_self());

    auto shop_itr = shop.find(template_id);

    if (shop_itr == shop.end()) {
        shop.emplace(CONTRACTN, [&](auto& item) {
            item.template_id = template_id;
            item.item_name = item_name;
            item.img = img;
            item.schema_name = schema_name;
            item.game = game;
            item.timestamp_start = timestamp_start;
            item.timestamp_end = timestamp_end;
            item.max_claimable = max_claimable;
            item.available_items = available_items;
            item.account_limit = account_limit;
            item.price = price;
            item.description = description;
            item.extra_data = extra_data;
        });
    } else {
        shop.modify(shop_itr, CONTRACTN, [&](auto& item) {
            item.item_name = item_name;
            item.img = img;
            item.schema_name = schema_name;
            item.game = game;
            item.timestamp_start = timestamp_start;
            item.timestamp_end = timestamp_end;
            item.max_claimable = max_claimable;
            item.available_items = available_items;
            item.account_limit = account_limit;
            item.price = price;
            item.description = description;
            item.extra_data = extra_data;
        });
    }
}

void clashdomewld::eraseshopit(
    uint32_t template_id
) {

    require_auth(get_self());

    auto shop_itr = shop.find(template_id);

    check(shop_itr != shop.end(), "Item with template " + to_string(template_id) + " doesn't exist!");

    shop.erase(shop_itr);
}

void clashdomewld::addtowl (
    uint32_t template_id,
    vector<name> accounts_to_add
) {
    require_auth(get_self());

    auto shop_itr = shop.find(template_id);

    check(shop_itr != shop.end(), "Item with template " + to_string(template_id) + " doesn't exist!");

    shop.modify(shop_itr, CONTRACTN, [&](auto& row) {
        row.whitelist.insert(row.whitelist.end(), accounts_to_add.begin(), accounts_to_add.end());
    });
}

void clashdomewld::erasefromwl (
    uint32_t template_id,
    name account_to_remove
) {
    require_auth(get_self());

    auto shop_itr = shop.find(template_id);

    check(shop_itr != shop.end(), "Item with template " + to_string(template_id) + " doesn't exist!");

    uint64_t pos = finder(shop_itr->whitelist, account_to_remove);

    if (pos != -1) {
        shop.modify(shop_itr, CONTRACTN, [&](auto& row) {
            row.whitelist.erase(row.whitelist.begin() + pos);
        });
    }
}

void clashdomewld::clearwl (
    uint32_t template_id
) {
    require_auth(get_self());

    auto shop_itr = shop.find(template_id);

    check(shop_itr != shop.end(), "Item with template " + to_string(template_id) + " doesn't exist!");

    vector<name> empty_v;

    shop.modify(shop_itr, CONTRACTN, [&](auto& row) {
        row.whitelist = empty_v;
    });
}


void clashdomewld::setaclimitwl (
    uint32_t template_id,
    int32_t account_limit
) {
    require_auth(get_self());

    auto shop_itr = shop.find(template_id);

    check(shop_itr != shop.end(), "Item with template " + to_string(template_id) + " doesn't exist!");

    shop.modify(shop_itr, CONTRACTN, [&](auto& row) {
        row.account_limit = account_limit;
    });
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

void clashdomewld::setcraft(
    uint32_t template_id,
    vector<asset> craft
) {
    require_auth(get_self());

    auto tool_conf_itr = toolconfig.find(template_id);
    check(tool_conf_itr != toolconfig.end(), "Tool with template id " + to_string(template_id) + " doesn't exist!");

    toolconfig.modify(tool_conf_itr, CONTRACTN, [&](auto& config_row) {
        config_row.craft = craft;
    });
}

void clashdomewld::setslotcraft(
    uint32_t template_id,
    vector<asset> craft
) {
    require_auth(get_self());

    auto slots_conf_itr = slotsconfig.find(template_id);
    check(slots_conf_itr != slotsconfig.end(), "Slot with template id " + to_string(template_id) + " doesn't exist!");

    slotsconfig.modify(slots_conf_itr, CONTRACTN, [&](auto& config_row) {
        config_row.craft = craft;
    });
}

void clashdomewld::setreward(
    uint32_t template_id,
    vector<asset> rewards
) {
    require_auth(get_self());

    auto tool_conf_itr = toolconfig.find(template_id);
    check(tool_conf_itr != toolconfig.end(), "Tool with template id " + to_string(template_id) + " doesn't exist!");

    toolconfig.modify(tool_conf_itr, CONTRACTN, [&](auto& config_row) {
        config_row.rewards = rewards;
    });
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

void clashdomewld::eraseaccount(
    name account
) {

    require_auth(get_self());

    auto acc_itr = accounts.find(account.value);

    check(acc_itr != accounts.end(), "Account " + account.to_string() + " doesn't exist!");

    accounts.erase(acc_itr);
}

void clashdomewld::erasecitiz(
    name account
) {

    require_auth(get_self());

    auto acc_itr = citiz.find(account.value);

    check(acc_itr != citiz.end(), "Account " + account.to_string() + " doesn't exist!");

    citiz.erase(acc_itr);
}

void clashdomewld::erasetrial(
    name account
) {

    require_auth(get_self());

    auto acc_itr = trials.find(account.value);

    check(acc_itr != trials.end(), "Account " + account.to_string() + " doesn't exist!");

    trials.erase(acc_itr);
}

void clashdomewld::erasetable(
    string table_name
) {
    require_auth(get_self());
    //require_auth(name("packsopenerx"));

    if (table_name == "accounts") {
        for (auto itr = accounts.begin(); itr != accounts.end();) {
            itr = accounts.erase(itr);
        }
    } else if (table_name == "citiz") {
        for (auto itr = citiz.begin(); itr != citiz.end();) {
            itr = citiz.erase(itr);
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
    } else if (table_name == "tokenstats") {
        for (auto itr = tokenstats.begin(); itr != tokenstats.end();) {
            itr = tokenstats.erase(itr);
        }
    } else if (table_name == "trials") {
        for (auto itr = trials.begin(); itr != trials.end();) {
            itr = trials.erase(itr);
        }
    } else if (table_name == "partners") {
        for (auto itr = partners.begin(); itr != partners.end();) {
            itr = partners.erase(itr);
        }
    } else if (table_name == "smclaim") {
        for (auto itr = smclaim.begin(); itr != smclaim.end();) {
            itr = smclaim.erase(itr);
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
        acc.unclaimed_credits = unclaimed_credits;
        acc.unclaimed_actions = unclaimed_actions;
    });
}

void clashdomewld::settrialcr(
    name account,
    asset credits
) {

    require_auth(get_self());

    auto ac_itr = trials.find(account.value);

    check(ac_itr != trials.end(), "Account " + account.to_string() + " doesn't exist!");

    trials.modify(ac_itr, CONTRACTN, [&](auto& acc) {
        acc.credits = credits;
        acc.full = credits.amount >= TRIAL_MAX_UNCLAIMED;
    });
}

void clashdomewld::setdecdata(
    uint64_t asset_id,
    name account,
    string data
) {

    require_auth(account);

    auto decoration_itr = decorations.find(asset_id);

    check(decoration_itr != decorations.end(), "Decoration with id " + to_string(asset_id) + " doesn't exist!");
    check(decoration_itr->owner == account, "Account " + account.to_string() + " isn't the owner of asset " + to_string(asset_id));

    decorations.modify(decoration_itr, CONTRACTN, [&](auto& decoration) {
        decoration.data = data;
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
                    config_row.accounts = {name("clashdomeali"), name("clashdomebob")};
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

    citiz.emplace(CONTRACTN, [&](auto& acc) {
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

    uint16_t initial_free_slots_value = 1;

    accounts.emplace(CONTRACTN, [&](auto& acc) {
        acc.account = account;
        acc.stamina = 100;
        acc.battery = config_itr->init_battery;
        acc.carbz_free_slots = initial_free_slots_value;
        acc.jigowatts_free_slots = initial_free_slots_value;
        acc.carbz_slots = initial_free_slots_value;
        acc.jigowatts_slots = initial_free_slots_value;
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
    uint64_t max_value = 27;
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

                asset quantity2;
            
                quantity2.amount = quantities[i].amount * 2;
                quantity2.symbol = quantities[i].symbol;

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
                //update daily token stats

                if (quantities[i].symbol == CDJIGO_SYMBOL) {
                    quantities[i].symbol = JIGOWATTS_SYMBOL;
                } else if (quantities[i].symbol == LUDIO_SYMBOL) {
                    quantities[i].symbol = CREDITS_SYMBOL;
                } else if (quantities[i].symbol == CDCARBZ_SYMBOL) {
                    quantities[i].symbol = CARBZ_SYMBOL;
                }

                updateDailyStats(quantities[i],1);

            }
        }

    } else if (points < 0) {

        winner = rival_name;

        for (auto i = 0; i < quantities.size(); i++) {
            if (quantities[i].amount > 0) {

                action (
                    permission_level{get_self(), name("active")},
                    name("clashdometkn"),
                    name("burn"),
                    std::make_tuple(
                        quantities[i],
                        "Gigaswap " + quantities[i].to_string()
                    )
                ).send();

                if (quantities[i].symbol == CDJIGO_SYMBOL) {
                    quantities[i].symbol = JIGOWATTS_SYMBOL;
                } else if (quantities[i].symbol == LUDIO_SYMBOL) {
                    quantities[i].symbol = CREDITS_SYMBOL;
                } else if (quantities[i].symbol == CDCARBZ_SYMBOL) {
                    quantities[i].symbol = CARBZ_SYMBOL;
                }

                //update daily token stats
                updateDailyStats(quantities[i],2);
                updateDailyStats(quantities[i],0);

            }
        }
    } else {

        for (auto i = 0; i < quantities.size(); i++) {
            if (quantities[i].amount > 0) {

                action(
                    permission_level{get_self(), name("active")},
                    name("clashdometkn"),
                    name("transfer"),
                    std::make_tuple(
                        get_self(),
                        account,
                        quantities[i],
                        "Gigaswap " + account.to_string()
                    )
                ).send();
                //update daily token stats

                if (quantities[i].symbol == CDJIGO_SYMBOL) {
                    quantities[i].symbol = JIGOWATTS_SYMBOL;
                } else if (quantities[i].symbol == LUDIO_SYMBOL) {
                    quantities[i].symbol = CREDITS_SYMBOL;
                } else if (quantities[i].symbol == CDCARBZ_SYMBOL) {
                    quantities[i].symbol = CARBZ_SYMBOL;
                }
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

void clashdomewld::addavatar(
    name acount,
    string avatar
) {
    require_auth(name("packsopenerx"));

    auto citizen_itr = citiz.find(acount.value);

    citiz.modify(citizen_itr, CONTRACTN, [&](auto& citizen) {
        citizen.avatar = avatar;
    });
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
    } else if (memo == "stake decoration") {
        stakeDecoration(asset_ids[0], from, to);
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

    if (memo.find("repair_int") != string::npos) {

        auto config_itr = config.begin();

        const size_t fb = memo.find(":");
        string d1 = memo.substr(0, fb);
        string d2 = memo.substr(fb + 1);

        uint64_t asset_id = stoull(d2);

        auto ac_itr = accounts.find(from.value);
        check(ac_itr != accounts.end(), "Account with name " + from.to_string() + " doesn't exist!");

        auto tool_itr = tools.find(asset_id);
        check(tool_itr != tools.end(), "Tool with id " + to_string(asset_id) + " doesn't exist!");
        
        check(tool_itr->owner == from, "Account " + from.to_string() + " isn't the owner of asset " + to_string(asset_id));

        auto tool_conf_itr = toolconfig.find(tool_itr->template_id);
        check(tool_conf_itr != toolconfig.end(), "Tool with template id " + to_string(tool_itr->template_id) + " doesn't exist!");

        check(quantity.symbol == LUDIO_SYMBOL, "Invalid symbol.");

        uint16_t integrity_necessary = tool_itr->max_integrity - tool_itr->integrity;

        check(integrity_necessary > 0, "No need to repair Integrity.");
        
        check(quantity.amount * config_itr->credits_to_integrity == integrity_necessary * 10000, "Invalid Credits amount.");

        tools.modify(tool_itr, CONTRACTN, [&](auto& tool) {
            tool.integrity += integrity_necessary;
        });

        //update daily token stats
        asset update_stats_asset;
        update_stats_asset.amount=(integrity_necessary * 10000) / config_itr->credits_to_integrity;
        update_stats_asset.symbol=CREDITS_SYMBOL;
        updateDailyStats(update_stats_asset,0);

    } else if (memo.find("repair_bat") != string::npos) {

        auto config_itr = config.begin();

        const size_t fb = memo.find(":");
        string d1 = memo.substr(0, fb);
        string d2 = memo.substr(fb + 1);

        uint64_t asset_id = stoull(d2);

        auto ac_itr = accounts.find(from.value);
        check(ac_itr != accounts.end(), "Account with name " + from.to_string() + " doesn't exist!");

        auto tool_itr = tools.find(asset_id);
        check(tool_itr != tools.end(), "Tool with id " + to_string(asset_id) + " doesn't exist!");
        
        check(tool_itr->owner == from, "Account " + from.to_string() + " isn't the owner of asset " + to_string(asset_id));

        auto tool_conf_itr = toolconfig.find(tool_itr->template_id);
        check(tool_conf_itr != toolconfig.end(), "Tool with template id " + to_string(tool_itr->template_id) + " doesn't exist!");

        check(quantity.symbol == CDJIGO_SYMBOL, "Invalid symbol.");

        uint16_t battery_necessary = tool_itr->max_battery - tool_itr->battery;

        check(battery_necessary > 0, "No need to repair Battery.");

        check(quantity.amount * config_itr->jigowatts_to_battery == battery_necessary * 10000, "Invalid Jigowatts amount.");

        tools.modify(tool_itr, CONTRACTN, [&](auto& tool) {
            tool.battery += battery_necessary;
        });

        //update daily token stats
        asset update_stats_asset;
        update_stats_asset.amount=(battery_necessary * 10000) / config_itr->jigowatts_to_battery;
        update_stats_asset.symbol=JIGOWATTS_SYMBOL;
        updateDailyStats(update_stats_asset,0);

    } else if (memo == "repair_acc_bat") {

        auto config_itr = config.begin();

        auto ac_itr = accounts.find(from.value);
        check(ac_itr != accounts.end(), "Account with name " + from.to_string() + " doesn't exist!");

        check(quantity.symbol == CDJIGO_SYMBOL, "Invalid symbol.");

        check(ac_itr->battery < config_itr->max_battery, "You already have maximum battery.");

        uint16_t battery = config_itr->max_battery - ac_itr->battery;
        
        check(quantity.amount * config_itr->jigowatts_to_battery == battery * 10000, "Invalid Jigowatts amount.");

        accounts.modify(ac_itr, CONTRACTN, [&](auto& acc) {
            acc.battery += battery;
        });
    } else if(memo == "repair_stamina") {
        auto config_itr = config.begin();

        auto ac_itr = accounts.find(from.value);
        check(ac_itr != accounts.end(), "Account with name " + from.to_string() + " doesn't exist!");

        auto citizen_itr = citiz.find(from.value);
        check(citizen_itr != citiz.end(), "Account with name " + from.to_string() + " doesn't has an citizen!");

        auto citizconfig_itr = citizconfig.find(citizen_itr->type);

        check(quantity.symbol == CDCARBZ_SYMBOL, "Invalid symbol.");

        check(ac_itr->stamina < citizconfig_itr->max_stamina, "You already have maximum stamina.");

        uint16_t stamina = citizconfig_itr->max_stamina - ac_itr->stamina;
        
        check(quantity.amount * config_itr->carbz_to_stamina == stamina * 10000, "Invalid Carbz amount.");

        accounts.modify(ac_itr, CONTRACTN, [&](auto& acc) {
            acc.stamina += stamina;
        });

        //update daily token stats
        asset update_stats_asset;
        update_stats_asset.amount=(stamina * 10000) / config_itr->carbz_to_stamina;
        update_stats_asset.symbol=CARBZ_SYMBOL;
        updateDailyStats(update_stats_asset,0);

    } else if (memo.find("social") != string::npos) {

        check(quantity.symbol == CDCARBZ_SYMBOL, "Invalid token symbol.");
        check(quantity.amount == SOCIAL_CARBZ_PAYMENT, "Invalid token amount.");
        parseSocialsMemo(from, memo);
    }else if (memo.find("earn") != string::npos ){

        const size_t fb = memo.find(":");
        string d1 = memo.substr(0, fb); //"earn"
        string d2 = memo.substr(fb + 1); // type

        earnstake(from,quantity,stoi(d2));
    } else {
        check(memo == "transfer", "Invalid memo.");
    }
}


void clashdomewld::receive_tokens_transfer(
    name from,
    name to,
    vector<asset> quantities,
    string memo
) {


    if (to != get_self()) {
        return;
    }
    if(memo.find("gigaswap") != string::npos) {

        const size_t fb = memo.find(":");
        string d1 = memo.substr(0, fb);

        string d2 = memo.substr(fb + 1);

        const size_t fb2 = d2.find(":");

        string str_timestamp = d2.substr(0, fb2);
        string str_choices = d2.substr(fb2 + 1);

        uint64_t timestamp = (uint64_t) stoull(str_timestamp);
        uint64_t str_length = str_choices.length();

        vector<uint8_t> choices = {0,0,0};

        uint64_t j = 0, i;

        for (i = 0; str_choices[i] != '\0'; i++) {

            if (str_choices[i] == ','){
                j++;
            }
            else {
                choices[j] = choices[j] * 10 + (str_choices[i] - 48);
            }
        }

        auto ac_itr = accounts.find(from.value);
        check(ac_itr != accounts.end(), "Account with name " + from.to_string() + " doesn't exist!");

        check(choices.size() == 3, "Invalid choices length.");

        for (auto i = 0; i < choices.size(); i++) {
            check(choices[i] >= 0 && choices[i] <= 2, "Invalid choices. Must be a number between 0 and 2.");
        }

        for (auto i = 0; i < quantities.size(); i++) {
            if (quantities[i].amount > 0) {
                check(quantities[i].amount <= 5000000, "Sorry. GigaSwap is limited to 500 tokens");
            }
        }

        auto size = transaction_size();
        char buf[size];

        auto read = read_transaction(buf, size);
        check(size == read, "read_transaction() has failed.");

        checksum256 tx_id = eosio::sha256(buf, read);

        uint64_t signing_value;

        memcpy(&signing_value, tx_id.data(), sizeof(signing_value));

        auto gs_itr = gigaswap.find(from.value);

        vector<uint8_t> opponent_choices;

        if (gs_itr == gigaswap.end()) {

            gigaswap.emplace(CONTRACTN, [&](auto& swap) {
                swap.account = from;
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
                from.value, //used as assoc id
                signing_value,
                get_self()
            )
        ).send();

    } else if(memo.find("craft_tool") != string::npos) {

        check(0 == 1, "Craft tool currently disabled.");

        const size_t fb = memo.find(":");
        string d1 = memo.substr(0, fb);
        string d2 = memo.substr(fb + 1);

        uint32_t template_id = (uint32_t) stoull(d2);

        auto ac_itr = accounts.find(from.value);
        check(ac_itr != accounts.end(), "Account with name " + from.to_string() + " doesn't exist!");

        auto citizen_itr = citiz.find(from.value);
        check(citizen_itr != citiz.end(), "Account with name " + from.to_string() + " has no citizen staked!");

        auto tool_itr = toolconfig.find(template_id);
        check(tool_itr != toolconfig.end(), "Tool with template id " + to_string(template_id) + " doesn't exist!");

        // check all necesary materials && remove materials

        for(auto i = 0; i < tool_itr->craft.size(); i++) {

            uint64_t pos = finder(quantities, tokenConversion(tool_itr->craft[i].symbol));

            check(pos != -1, "Insufficient materials.");
            check(quantities[pos].amount >= tool_itr->craft[i].amount, "Insufficient materials.");

            burnTokens(tool_itr->craft[i], "tool with template_id " + to_string(template_id) + ".");

            //update daily token stats
            updateDailyStats(tool_itr->craft[i],0);
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
                from,
                (atomicassets::ATTRIBUTE_MAP) {},
                (atomicassets::ATTRIBUTE_MAP) {},
                (vector <asset>) {}
            )
        ).send();
    } else if (memo.find("craft_slot") != string::npos) {

        const size_t fb = memo.find(":");
        string d1 = memo.substr(0, fb);
        string d2 = memo.substr(fb + 1);

        uint32_t template_id = (uint32_t) stoull(d2);

        auto ac_itr = accounts.find(from.value);
        check(ac_itr != accounts.end(), "Account with name " + from.to_string() + " doesn't exist!");

        auto citizen_itr = citiz.find(from.value);
        check(citizen_itr != citiz.end(), "Account with name " + from.to_string() + " has no citizen staked!");

        auto slot_itr = slotsconfig.find(template_id);
        check(slot_itr != slotsconfig.end(), "Slot with template id " + to_string(template_id) + " doesn't exist!");

        // check all necesary materials && remove materials

        for(auto i = 0; i < slot_itr->craft.size(); i++) {

            uint64_t pos = finder(quantities, tokenConversion(slot_itr->craft[i].symbol));

            check(pos != -1, "Insufficient materials.");
            check(quantities[pos].amount >= slot_itr->craft[i].amount, "Insufficient materials.");

            burnTokens(slot_itr->craft[i], "slot with template_id " + to_string(template_id) + ".");
            
            //update daily token stats
            updateDailyStats(slot_itr->craft[i],0);
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
                from,
                (atomicassets::ATTRIBUTE_MAP) {},
                (atomicassets::ATTRIBUTE_MAP) {},
                (vector <asset>) {}
            )
        ).send();
    } else if (memo.find("craft_wallet") != string::npos) {

        check(0 == 1, "Craft wallet currently disabled.");

        const size_t fb = memo.find(":");
        string d1 = memo.substr(0, fb);
        string d2 = memo.substr(fb + 1);

        uint32_t template_id = (uint32_t) stoull(d2);

        auto ac_itr = accounts.find(from.value);
        check(ac_itr != accounts.end(), "Account with name " + from.to_string() + " doesn't exist!");

        auto citizen_itr = citiz.find(from.value);
        check(citizen_itr != citiz.end(), "Account with name " + from.to_string() + " has no citizen staked!");

        auto wallet_itr = walletconfig.find(template_id);
        check(wallet_itr != walletconfig.end(), "Wallet with template id " + to_string(template_id) + " doesn't exist!");

        // check all necesary materials && remove materials

        for(auto i = 0; i < wallet_itr->craft.size(); i++) {

            uint64_t pos = finder(quantities, tokenConversion(wallet_itr->craft[i].symbol));

            check(pos != -1, "Insufficient materials.");
            check(quantities[pos].amount >= wallet_itr->craft[i].amount, "Insufficient materials.");

            burnTokens(wallet_itr->craft[i], "wallet with template_id " + to_string(template_id) + ".");
            
            //update daily token stats
            updateDailyStats(wallet_itr->craft[i],0);
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
                from,
                (atomicassets::ATTRIBUTE_MAP) {},
                (atomicassets::ATTRIBUTE_MAP) {},
                (vector <asset>) {}
            )
        ).send();
    } else if(memo.find("buy_item") != string::npos) {

        const size_t fb = memo.find(":");
        string d1 = memo.substr(0, fb);
        string d2 = memo.substr(fb + 1);

        uint32_t template_id = (uint32_t) stoull(d2);

        auto shop_itr = shop.find(template_id);
        check(shop_itr != shop.end(), "Item with template id " + to_string(template_id) + " doesn't exist!");

        uint64_t pos = finder(shop_itr->whitelist, from);

        check(pos != -1 || shop_itr->whitelist.size() == 0, "Account " + from.to_string() + " isn't in the whitelist.");

        check(shop_itr->max_claimable == -1 || shop_itr->available_items > 0, "Item " + to_string(template_id) + " is out of stock!");

        uint64_t timestamp = eosio::current_time_point().sec_since_epoch();

        check(shop_itr->timestamp_start < timestamp, "Item " + to_string(template_id) + " isn't available yet!");
        check(shop_itr->timestamp_end > timestamp, "Item " + to_string(template_id) + " is no longer available!");

        auto smclaim_itr = smclaim.find(from.value);

        if (smclaim_itr == smclaim.end()) {
            smclaim.emplace(CONTRACTN, [&](auto& acc) {
                acc.account = from;
                acc.template_id = template_id;
                acc.claims = 1;
            });
        } else {

            if (smclaim_itr->template_id != template_id) {
                smclaim.modify(smclaim_itr, CONTRACTN, [&](auto& acc) {
                    acc.template_id = template_id;
                    acc.claims = 1;
                });
            } else {
                check(smclaim_itr->claims < shop_itr->account_limit, "You have obtained the maximum number of items.");

                smclaim.modify(smclaim_itr, CONTRACTN, [&](auto& acc) {
                    acc.template_id = template_id;
                    acc.claims++;
                });
            }
        }

        for(auto i = 0; i < shop_itr->price.size(); i++) {

            uint64_t pos = finder(quantities, shop_itr->price[i].symbol);

            check(pos != -1, "Insufficient materials.");
            check(quantities[pos].amount == shop_itr->price[i].amount, "Invalid materials.");

            // TODO: decidir si se quema o no al comprar
            // burnTokens(shop_itr->price[i], "Item with template_id " + to_string(template_id) + ".");
            
            updateDailyStats(shop_itr->price[i],0);
        }

        if (shop_itr->max_claimable != -1) {
            shop.modify(shop_itr, CONTRACTN, [&](auto& item) {
                item.available_items--;
            });
        } else {
            shop.modify(shop_itr, CONTRACTN, [&](auto& item) {
                item.available_items++;
            });
        }

        // mint and send item
        action (
            permission_level{get_self(), name("active")},
            atomicassets::ATOMICASSETS_ACCOUNT,
            name("mintasset"),
            std::make_tuple(
                get_self(),
                name(COLLECTION_NAME),
                shop_itr->schema_name,
                shop_itr->template_id,
                from,
                (atomicassets::ATTRIBUTE_MAP) {},
                (atomicassets::ATTRIBUTE_MAP) {},
                (vector <asset>) {}
            )
        ).send();
    }
}

void clashdomewld::receive_wax_transfer(
    name from,
    name to,
    asset quantity,
    string memo
) {

    if (to != get_self()) {
        return;
    }

    if (memo.find("buy_item") != string::npos) {

        const size_t fb = memo.find(":");
        string d1 = memo.substr(0, fb);
        string d2 = memo.substr(fb + 1);

        uint32_t template_id = (uint32_t) stoull(d2);

        auto shop_itr = shop.find(template_id);
        check(shop_itr != shop.end(), "Item with template id " + to_string(template_id) + " doesn't exist!");

        check(shop_itr->max_claimable == -1 || shop_itr->available_items > 0, "Item " + to_string(template_id) + " is out of stock!");

        auto smclaim_itr = smclaim.find(from.value);

        if (smclaim_itr == smclaim.end()) {
            smclaim.emplace(CONTRACTN, [&](auto& acc) {
                acc.account = from;
                acc.template_id = template_id;
                acc.claims = 1;
            });
        } else {

            if (smclaim_itr->template_id != template_id) {
                smclaim.modify(smclaim_itr, CONTRACTN, [&](auto& acc) {
                    acc.template_id = template_id;
                    acc.claims = 1;
                });
            } else {
                check(smclaim_itr->claims < shop_itr->account_limit, "You have obtained the maximum number of items.");

                smclaim.modify(smclaim_itr, CONTRACTN, [&](auto& acc) {
                    acc.template_id = template_id;
                    acc.claims++;
                });
            }
        }

        check(quantity.amount == shop_itr->price[0].amount, "Invalid WAX amount.");

        uint64_t timestamp = eosio::current_time_point().sec_since_epoch();

        check(shop_itr->timestamp_start < timestamp, "Item " + to_string(template_id) + " isn't available yet!");
        check(shop_itr->timestamp_end > timestamp, "Item " + to_string(template_id) + " is no longer available!");

        if (shop_itr->max_claimable != -1) {
            shop.modify(shop_itr, CONTRACTN, [&](auto& item) {
                item.available_items--;
            });
        } else {
            shop.modify(shop_itr, CONTRACTN, [&](auto& item) {
                item.available_items++;
            });
        }

        // mint and send item
        action (
            permission_level{get_self(), name("active")},
            atomicassets::ATOMICASSETS_ACCOUNT,
            name("mintasset"),
            std::make_tuple(
                get_self(),
                name(COLLECTION_NAME),
                shop_itr->schema_name,
                shop_itr->template_id,
                from,
                (atomicassets::ATTRIBUTE_MAP) {},
                (atomicassets::ATTRIBUTE_MAP) {},
                (vector <asset>) {}
            )
        ).send();
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

uint64_t clashdomewld::finder(vector<name> whitelist, name account)
{
    for (uint64_t i = 0; i < whitelist.size(); i++)
    {
        if (whitelist.at(i) == account)
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

    auto citizen_itr = citiz.find(from.value);
    check(citizen_itr == citiz.end(), "Account with name " + from.to_string() + " already has an citizen!");

    atomicassets::assets_t player_assets = atomicassets::get_assets(to);
    auto asset_itr = player_assets.require_find(asset_id, "No NFT with this ID exists");

    // CHECK THAT THE ASSET CORRESPONDS TO OUR COLLECTION
    check(asset_itr->collection_name == name(COLLECTION_NAME), "NFT doesn't correspond to " + COLLECTION_NAME);
    check(asset_itr->schema_name == name(CITIZEN_SCHEMA_NAME), "NFT doesn't correspond to schema " + CITIZEN_SCHEMA_NAME);
    check(asset_itr->template_id != TRIAL_TEMPLATE_ID, "You can't stake a trial citizen.");

    atomicassets::schemas_t collection_schemas = atomicassets::get_schemas(name(COLLECTION_NAME));
    auto schema_itr = collection_schemas.find(name(CITIZEN_SCHEMA_NAME).value);

    atomicassets::templates_t collection_templates = atomicassets::get_templates(name(COLLECTION_NAME));
    auto template_itr = collection_templates.find(asset_itr->template_id);

    vector <uint8_t> immutable_serialized_data = template_itr->immutable_serialized_data;
    vector <uint8_t> mutable_serialized_data = asset_itr->mutable_serialized_data;

    atomicassets::ATTRIBUTE_MAP idata = atomicdata::deserialize(immutable_serialized_data, schema_itr->format);
    atomicassets::ATTRIBUTE_MAP mdata = atomicdata::deserialize(mutable_serialized_data, schema_itr->format);

    string rarity = get<string> (idata["rarity"]);

    string avatar;
    
    if (mdata.find("avatar") == mdata.end()) {
        avatar = get<string> (idata["avatar"]);
    } else {
        avatar = get<string> (mdata["avatar"]);
    } 

    uint8_t type = 0;

    if (rarity == "Pleb") {
        type = 0;
    } else if (rarity == "UberNorm") {
        type = 1;
    } else if (rarity == "Hi-Clone") {
        type = 2;
    }

    citiz.emplace(CONTRACTN, [&](auto& acc) {
        acc.account = from;
        acc.type = type;
        acc.citizen_id = asset_id;
        acc.avatar = avatar;
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

    burnTrial(from);
}

void clashdomewld::stakeTool(uint64_t asset_id, name from, name to)
{

    auto ac_itr = accounts.find(from.value);
    check(ac_itr != accounts.end(), "Stake a citizen first!!");

    auto citizen_itr = citiz.find(from.value);
    check(citizen_itr != citiz.end(), "Stake a citizen first!");

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

void clashdomewld::stakeSlot(uint64_t asset_id, name from, name to, string type)
{

    auto ac_itr = accounts.find(from.value);
    check(ac_itr != accounts.end(), "Stake a citizen first!!");

    auto citizen_itr = citiz.find(from.value);
    check(citizen_itr != citiz.end(), "Stake a citizen first!");

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

void clashdomewld::stakeWallet(uint64_t asset_id, name from, name to)
{

    auto ac_itr = accounts.find(from.value);
    check(ac_itr != accounts.end(), "Stake a citizen first!!");

    auto citizen_itr = citiz.find(from.value);
    check(citizen_itr != citiz.end(), "Stake a citizen first!");

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

void clashdomewld::stakeDecoration(uint64_t asset_id, name from, name to)
{

    auto ac_itr = accounts.find(from.value);
    check(ac_itr != accounts.end(), "Stake a citizen first!!");

    auto citizen_itr = citiz.find(from.value);
    check(citizen_itr != citiz.end(), "Stake a citizen first!");

    atomicassets::assets_t player_assets = atomicassets::get_assets(to);
    auto asset_itr = player_assets.require_find(asset_id, "No NFT with this ID exists");

    // CHECK THAT THE ASSET CORRESPONDS TO OUR COLLECTION
    check(asset_itr->collection_name == name(COLLECTION_NAME), "NFT doesn't correspond to " + COLLECTION_NAME);
    check(asset_itr->schema_name == name(DECORATION_SCHEMA_NAME), "NFT doesn't correspond to schema " + DECORATION_SCHEMA_NAME);

    // auto decoration_idx = decorations.get_index<name("byowner")>();
    // auto decoration_itr =  decoration_idx.lower_bound(from.value);

    // uint64_t count = 0;

    // while (decoration_itr != decoration_idx.end() && decoration_itr->owner == from) {
    //     count++;
    //     decoration_itr++;
    // } 

    // check(count < 3, "Maximum decoration elements = 3.");

    decorations.emplace(CONTRACTN, [&](auto& decoration) {
        decoration.asset_id = asset_id;
        decoration.owner = from;
        decoration.template_id = asset_itr->template_id;
    });
}

void clashdomewld::getTokens(uint64_t asset_id, name from, name to)
{

    auto ac_itr = accounts.find(from.value);
    check(ac_itr != accounts.end(), "Stake a citizen first!!");

    auto citizen_itr = citiz.find(from.value);
    check(citizen_itr != citiz.end(), "Stake a citizen first!");

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

    vector<asset> quantities;

    asset jigo;
    jigo.symbol = CDJIGO_SYMBOL;
    jigo.amount = PACK_JIGO_REWARD;

    quantities.push_back(jigo);

    asset carbz;
    carbz.symbol = CDCARBZ_SYMBOL;
    carbz.amount = PACK_CARBZ_REWARD;

    quantities.push_back(carbz);

    action(
        permission_level{get_self(), name("active")},
        name("clashdometkn"),
        name("transfers"),
        std::make_tuple(
            get_self(),
            from,
            quantities,
            "Tokens pack reward " + from.to_string() + "."
        )
    ).send();
    
    //update Tokenomics
    asset temp;
    temp.symbol = JIGOWATTS_SYMBOL;
    temp.amount=PACK_JIGO_REWARD;
    updateDailyStats(temp,1);
    temp.symbol= CARBZ_SYMBOL;
    temp.amount=PACK_CARBZ_REWARD;
    updateDailyStats(temp,1);

}

void clashdomewld::burnTokens(asset tokens, string memo_extra)
{
    asset credits;
    credits.symbol = tokens.symbol;
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

    credits.symbol=tokens.symbol;
    updateDailyStats(credits,2);
}

void clashdomewld::burnTrial(name account)
{

    auto trial_itr = trials.find(account.value);
    auto ac_itr = accounts.find(account.value);

    if (trial_itr != trials.end()) {

        accounts.modify(ac_itr, CONTRACTN, [&](auto& acc) {
            acc.unclaimed_credits.amount += trial_itr->credits.amount;
        });

        // TODO: consumir stamina y bateria?

        trials.erase(trial_itr);
    }
}

void clashdomewld::checkEarlyAccess(name account, uint64_t early_access) {

    atomicassets::assets_t assets = atomicassets::get_assets(account);

    auto assets_itr = assets.find(early_access);

    check(assets_itr != assets.end(), "Account " + account.to_string() + " doesn't have the required NFT to play!");
    check(assets_itr->template_id == EARLY_ACCESS_TEMPLATE_ID, "The id provided does not correspond to the required NFT to play!");
}

void clashdomewld::parseSocialsMemo(name account, string memo)
{

    // memo: "social{cn:"pepe",co:"es",tw:"escrichee"}"

    // remove social from memo
    memo.erase(0,6);

    check(memo.length() < 200, "Memo maximum length.");
    
    auto social_data = json::parse(memo);

    check(social_data[CUSTOM_NAME].get_ref<json::string_t&>().size() < 20, " Custom name maximum length = 20");
    check(social_data[COUNTRY].get_ref<json::string_t&>().size() < 20, " Country maximum length = 20");
    check(social_data[TWITTER].get_ref<json::string_t&>().size() < 20, " Twitter account maximum length = 20");
    check(social_data[TELEGRAM].get_ref<json::string_t&>().size() < 20, " Telegram account maximum length = 20");
    check(social_data[DISCORD].get_ref<json::string_t&>().size() < 20, " Discord account maximum length = 20");

    auto ac_itr = social.find(account.value);

    if (ac_itr == social.end()) {
        social.emplace(CONTRACTN, [&](auto& acc) {
            acc.account = account;
            acc.data = social_data.dump();
        });
    } else {
        social.modify(ac_itr, CONTRACTN, [&](auto& acc) {
            acc.data = social_data.dump();
        });
    }
}

void clashdomewld::updateDailyStats(asset assetVal,int type){
    int64_t amount= assetVal.amount;
    symbol symbol= assetVal.symbol;

    if (symbol == LUDIO_SYMBOL) {
        symbol = CREDITS_SYMBOL;
    } else if (symbol == CDCARBZ_SYMBOL) {
        symbol = CARBZ_SYMBOL;
    } else if (symbol == CDJIGO_SYMBOL) {
        symbol = JIGOWATTS_SYMBOL;
    }

    asset nullasset;
    nullasset.amount=0.0000;
    nullasset.symbol=CARBZ_SYMBOL;
    uint64_t timestamp = eosio::current_time_point().sec_since_epoch();
    
    uint32_t day=epochToDay(timestamp);

    auto ptokenstatsitr = tokenstats.find(day);

    if (ptokenstatsitr == tokenstats.end()) {

        tokenstats.erase(tokenstats.begin());                 //We delete the oldest row in the table

        ptokenstatsitr = tokenstats.emplace(CONTRACTN, [&](auto &new_d) {
            new_d.day = day;
            new_d.mined_carbz=nullasset;
            new_d.consumed_carbz=nullasset;
            new_d.burned_carbz=nullasset;
            nullasset.symbol=CREDITS_SYMBOL;
            new_d.mined_credits=nullasset;
            new_d.consumed_credits=nullasset;
            new_d.burned_credits=nullasset;
            nullasset.symbol=JIGOWATTS_SYMBOL;
            new_d.mined_jigo=nullasset;
            new_d.consumed_jigo=nullasset;
            new_d.burned_jigo=nullasset;
        });
    }
    if(symbol==CARBZ_SYMBOL){
        if (type==1){
        //mined carbz++
        int64_t currtoken=ptokenstatsitr->mined_carbz.amount;
        currtoken += amount;
        tokenstats.modify(ptokenstatsitr, get_self(), [&](auto &mod_day) {
                mod_day.mined_carbz.amount=currtoken;
            });
        
        }else if(type==0){
        //consumed carbz++
        int64_t currtoken=ptokenstatsitr->consumed_carbz.amount;
        currtoken += amount;
        tokenstats.modify(ptokenstatsitr, get_self(), [&](auto &mod_day) {
                mod_day.consumed_carbz.amount=currtoken;
            });
        }else if(type==2){
        //burned carbz++
        int64_t currtoken=ptokenstatsitr->burned_carbz.amount;
        currtoken += amount;
        tokenstats.modify(ptokenstatsitr, get_self(), [&](auto &mod_day) {
                mod_day.burned_carbz.amount=currtoken;
            });
        }
    }
    else if(symbol==CREDITS_SYMBOL){
        if (type==1){
        //mined credits++
        int64_t currtoken=ptokenstatsitr->mined_credits.amount;
        currtoken += amount;
        tokenstats.modify(ptokenstatsitr, get_self(), [&](auto &mod_day) {
                mod_day.mined_credits.amount=currtoken;
            });
        
        }else if(type==0){
        //consumed credits++
        int64_t currtoken=ptokenstatsitr->consumed_credits.amount;
        currtoken += amount;
        tokenstats.modify(ptokenstatsitr, get_self(), [&](auto &mod_day) {
                mod_day.consumed_credits.amount=currtoken;
            });
        }else if(type==2){
        //burned credits++
        int64_t currtoken=ptokenstatsitr->burned_credits.amount;
        currtoken += amount;
        tokenstats.modify(ptokenstatsitr, get_self(), [&](auto &mod_day) {
                mod_day.burned_credits.amount=currtoken;
            });
        }
    }
    else if(symbol==JIGOWATTS_SYMBOL){
        if (type==1){
        //minted jigo++
        int64_t currtoken=ptokenstatsitr->mined_jigo.amount;
        currtoken += amount;
        tokenstats.modify(ptokenstatsitr, get_self(), [&](auto &mod_day) {
                mod_day.mined_jigo.amount=currtoken;
            });
        
        }else if(type==0){
        //consumed jigo++
        int64_t currtoken=ptokenstatsitr->consumed_jigo.amount;
        currtoken += amount;
        tokenstats.modify(ptokenstatsitr, get_self(), [&](auto &mod_day) {
                mod_day.consumed_jigo.amount=currtoken;
            });
        }else if(type==2){
        //burned jigo++
        int64_t currtoken=ptokenstatsitr->burned_jigo.amount;
        currtoken += amount;
        tokenstats.modify(ptokenstatsitr, get_self(), [&](auto &mod_day) {
                mod_day.burned_jigo.amount=currtoken;
            });
        }
    }
} 

//earn program
void clashdomewld::earnstake(
    name account,
    asset staking,
    int8_t type
) { 

    check( 0< type && type <4, "Type out of range ");    

    float amount= staking.amount;
    symbol symbol= staking.symbol;

    check(symbol == CDCARBZ_SYMBOL || symbol== CDJIGO_SYMBOL || symbol == LUDIO_SYMBOL, "The stacked asset is not available");

    auto ptearntableitr = earntable.find(account.value);

    if (ptearntableitr == earntable.end()) {

        asset nullasset;
        nullasset.amount=0.0000;
        nullasset.symbol=LUDIO_SYMBOL;
        ptearntableitr = earntable.emplace(CONTRACTN, [&](auto &new_a) {
            new_a.account = account;

            new_a.staked_LUDIO = nullasset;
            new_a.timestamp_LUDIO = 0;
            new_a.APY_LUDIO = 0;

            nullasset.symbol = CDCARBZ_SYMBOL;
            new_a.staked_CDCARBZ = nullasset;
            new_a.timestamp_CDCARBZ=0;
            new_a.APY_CDCARBZ = 0;

            nullasset.symbol = CDJIGO_SYMBOL;
            new_a.staked_CDJIGO = nullasset;
            new_a.timestamp_CDJIGO = 0;
            new_a.APY_CDJIGO = 0;
            
        });
    }

    int APYs[3] = {1,2,3};
    uint64_t timestamp = eosio::current_time_point().sec_since_epoch();

    if(symbol == LUDIO_SYMBOL){

        earntable.modify(ptearntableitr, get_self(), [&](auto &mod_acc) {
                mod_acc.staked_LUDIO.amount += amount;
                mod_acc.timestamp_LUDIO= timestamp;
                mod_acc.APY_LUDIO = APYs[type];
            });

    }else if(symbol == CDCARBZ_SYMBOL){

         earntable.modify(ptearntableitr, get_self(), [&](auto &mod_acc) {
                mod_acc.staked_CDCARBZ.amount += amount;
                mod_acc.timestamp_CDCARBZ = timestamp;
                mod_acc.APY_CDCARBZ = APYs[type];
            });

    }else if ( symbol == CDJIGO_SYMBOL){

        earntable.modify(ptearntableitr, get_self(), [&](auto &mod_acc) {
                mod_acc.staked_CDJIGO.amount += amount;
                mod_acc.timestamp_CDJIGO = timestamp;
                mod_acc.APY_CDJIGO = APYs[type];
            });
    }
}

void clashdomewld::earnunstake(name account , string asset_name){

    require_auth(account);
    symbol asset_symbol = symbol(symbol_code(asset_name), 4);
    check(asset_symbol == CDCARBZ_SYMBOL || asset_symbol== CDJIGO_SYMBOL || asset_symbol == LUDIO_SYMBOL, "not valid asseet" );
    auto ptearntableitr = earntable.find(account.value);
    check( ptearntableitr != earntable.end(), "You don't have any asset staked");

    float stakedAmount;
    uint64_t stakingTimestamp;
    int APY;

    if (asset_symbol == LUDIO_SYMBOL){

        stakedAmount = ptearntableitr->staked_LUDIO.amount;
        stakingTimestamp = ptearntableitr->timestamp_LUDIO;
        APY = ptearntableitr->APY_LUDIO;
    }else if (asset_symbol == CDCARBZ_SYMBOL){

        stakedAmount = ptearntableitr->staked_CDCARBZ.amount;
        stakingTimestamp = ptearntableitr->timestamp_CDCARBZ;
        APY = ptearntableitr->APY_CDCARBZ;
    }else if (asset_symbol == CDJIGO_SYMBOL){

        stakedAmount = ptearntableitr->staked_CDJIGO.amount;
        stakingTimestamp = ptearntableitr->timestamp_CDJIGO;
        APY = ptearntableitr->APY_CDJIGO;
    }
    check(stakedAmount >0.0 && stakingTimestamp > 0 , "You have not staked that asset yet!" );
    float returns = getEarnReturns(stakedAmount, stakingTimestamp, APY); 

    asset quantity;
    
    quantity.symbol = asset_symbol;
    quantity.amount = returns;

    action(
        permission_level{get_self(), name("active")},
        name("clashdometkn"),
        name("transfer"),
        std::make_tuple(
            get_self(),
            account,
            quantity,
            "Withdraw " + account.to_string()
        )
    ).send();
}

float clashdomewld::getEarnReturns(float stakedAmount, uint64_t stakingTime, int APY){
    
    //APY to weeks map 
    std::map<int, int> APY_to_MinWeeks = {
        { 1, 1 },
        { 2, 2 },
        { 3, 4 }
        };

    uint64_t timestamp = eosio::current_time_point().sec_since_epoch();

    uint64_t staked_seconds = timestamp - stakingTime; //seconds 
    int  staked_weeks = floor(staked_seconds / (3600*24*7));
    int min_weeks = APY_to_MinWeeks[APY];

    if (staked_weeks >= min_weeks){//pagar + interesses

    float interest_per_week = 1 + APY/52;
    return stakedAmount * interest_per_week;

    }else{ //devolver sin intereses 
        return stakedAmount;
    } 
}

symbol clashdomewld::tokenConversion(symbol s1){

    if (s1 == CREDITS_SYMBOL) {
        return LUDIO_SYMBOL;
    } else if (s1 == CARBZ_SYMBOL) {
        return CDCARBZ_SYMBOL;
    } else if (s1 == JIGOWATTS_SYMBOL) {
        return CDJIGO_SYMBOL;
    }

    return s1;
}

uint32_t clashdomewld::epochToDay(time_t time){
    tm *tm_gmt = gmtime(&time);
	uint32_t daytime=0;
	return daytime=(tm_gmt->tm_year+1900)*10000+(tm_gmt->tm_mon+1)*100+(tm_gmt->tm_mday);
}
