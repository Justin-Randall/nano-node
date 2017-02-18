#include <gtest/gtest.h>
#include <rai/node/testing.hpp>
#include <rai/node/working.hpp>

#include <boost/make_shared.hpp>

TEST (node, stop)
{
    rai::system system (24000, 1);
    ASSERT_NE (system.nodes [0]->wallets.items.end (), system.nodes [0]->wallets.items.begin ());
    system.nodes [0]->stop ();
    system.service.run ();
    ASSERT_TRUE (true);
}

TEST (node, block_store_path_failure)
{
    rai::node_init init;
    auto service (boost::make_shared <boost::asio::io_service> ());
    rai::alarm alarm (*service);
	auto path (rai::unique_path ());
	rai::logging logging (path);
	rai::work_pool work (nullptr);
    auto node (std::make_shared <rai::node> (init, *service, 0, path, alarm, logging, work));
	ASSERT_TRUE (node->wallets.items.empty ());
    node->stop ();
}

TEST (node, inactive_supply)
{
    rai::node_init init;
    auto service (boost::make_shared <boost::asio::io_service> ());
	rai::alarm alarm (*service);
	auto path (rai::unique_path ());
	rai::node_config config (path);
	rai::work_pool work (nullptr);
	config.inactive_supply = 10;
    auto node (std::make_shared <rai::node> (init, *service, path, alarm, config, work));
	ASSERT_EQ (10, node->ledger.inactive_supply);
}

TEST (node, password_fanout)
{
    rai::node_init init;
    auto service (boost::make_shared <boost::asio::io_service> ());
	rai::alarm alarm (*service);
	auto path (rai::unique_path ());
	rai::node_config config (path);
	rai::work_pool work (nullptr);
	config.password_fanout = 10;
    auto node (std::make_shared <rai::node> (init, *service, path, alarm, config, work));
	auto wallet (node->wallets.create (100));
	ASSERT_EQ (10, wallet->store.password.values.size ());
}

TEST (node, balance)
{
    rai::system system (24000, 1);
	system.wallet (0)->insert_adhoc (rai::test_genesis_key.prv);
	rai::transaction transaction (system.nodes [0]->store.environment, nullptr, true);
	ASSERT_EQ (std::numeric_limits <rai::uint128_t>::max (), system.nodes [0]->ledger.account_balance (transaction, rai::test_genesis_key.pub));
}

TEST (node, representative)
{
    rai::system system (24000, 1);
	auto block1 (system.nodes [0]->representative (rai::test_genesis_key.pub));
	{
		rai::transaction transaction (system.nodes [0]->store.environment, nullptr, false);
		ASSERT_TRUE (system.nodes [0]->ledger.store.block_exists (transaction, block1));
	}
	rai::keypair key;
	ASSERT_TRUE (system.nodes [0]->representative (key.pub).is_zero ());
}

TEST (node, send_unkeyed)
{
    rai::system system (24000, 1);
    rai::keypair key2;
	system.wallet (0)->insert_adhoc (rai::test_genesis_key.prv);
	system.wallet (0)->store.password.value_set (rai::keypair ().prv);
    ASSERT_EQ (nullptr, system.wallet (0)->send_action (rai::test_genesis_key.pub, key2.pub, system.nodes [0]->config.receive_minimum.number ()));
}

TEST (node, send_self)
{
    rai::system system (24000, 1);
    rai::keypair key2;
	system.wallet (0)->insert_adhoc (rai::test_genesis_key.prv);
	system.wallet (0)->insert_adhoc (key2.prv);
    ASSERT_NE (nullptr, system.wallet (0)->send_action (rai::test_genesis_key.pub, key2.pub, system.nodes [0]->config.receive_minimum.number ()));
    auto iterations (0);
    while (system.nodes [0]->balance (key2.pub).is_zero ())
    {
        system.poll ();
        ++iterations;
        ASSERT_LT (iterations, 200);
    }
    ASSERT_EQ (std::numeric_limits <rai::uint128_t>::max () - system.nodes [0]->config.receive_minimum.number (), system.nodes [0]->balance (rai::test_genesis_key.pub));
}

TEST (node, send_single)
{
    rai::system system (24000, 2);
    rai::keypair key2;
	system.wallet (0)->insert_adhoc (rai::test_genesis_key.prv);
	system.wallet (1)->insert_adhoc (key2.prv);
    ASSERT_NE (nullptr, system.wallet (0)->send_action (rai::test_genesis_key.pub, key2.pub, system.nodes [0]->config.receive_minimum.number ()));
	ASSERT_EQ (std::numeric_limits <rai::uint128_t>::max () - system.nodes [0]->config.receive_minimum.number (), system.nodes [0]->balance (rai::test_genesis_key.pub));
	ASSERT_TRUE (system.nodes [0]->balance (key2.pub).is_zero ());
	auto iterations (0);
    while (system.nodes [0]->balance (key2.pub).is_zero ())
    {
        system.poll ();
        ++iterations;
        ASSERT_LT (iterations, 200);
    }
}

TEST (node, send_single_observing_peer)
{
    rai::system system (24000, 3);
    rai::keypair key2;
	system.wallet (0)->insert_adhoc (rai::test_genesis_key.prv);
	system.wallet (1)->insert_adhoc (key2.prv);
    ASSERT_NE (nullptr, system.wallet (0)->send_action (rai::test_genesis_key.pub, key2.pub, system.nodes [0]->config.receive_minimum.number ()));
	ASSERT_EQ (std::numeric_limits <rai::uint128_t>::max () - system.nodes [0]->config.receive_minimum.number (), system.nodes [0]->balance (rai::test_genesis_key.pub));
	ASSERT_TRUE (system.nodes [0]->balance (key2.pub).is_zero ());
	auto iterations (0);
    while (std::any_of (system.nodes.begin (), system.nodes.end (), [&] (std::shared_ptr <rai::node> const & node_a) { return node_a->balance (key2.pub).is_zero (); }))
    {
        system.poll ();
        ++iterations;
        ASSERT_LT (iterations, 200);
    }
}

TEST (node, send_single_many_peers)
{
    rai::system system (24000, 10);
    rai::keypair key2;
	system.wallet (0)->insert_adhoc (rai::test_genesis_key.prv);
	system.wallet (1)->insert_adhoc (key2.prv);
    ASSERT_NE (nullptr, system.wallet (0)->send_action (rai::test_genesis_key.pub, key2.pub, system.nodes [0]->config.receive_minimum.number ()));
	ASSERT_EQ (std::numeric_limits <rai::uint128_t>::max () - system.nodes [0]->config.receive_minimum.number (), system.nodes [0]->balance (rai::test_genesis_key.pub));
	ASSERT_TRUE (system.nodes [0]->balance (key2.pub).is_zero ());
	auto iterations (0);
    while (std::any_of (system.nodes.begin (), system.nodes.end (), [&] (std::shared_ptr <rai::node> const & node_a) { return node_a->balance (key2.pub).is_zero(); }))
    {
        system.poll ();
        ++iterations;
        ASSERT_LT (iterations, 2000);
    }
}

TEST (node, send_out_of_order)
{
    rai::system system (24000, 2);
    rai::keypair key2;
    rai::genesis genesis;
    rai::send_block send1 (genesis.hash (), key2.pub, std::numeric_limits <rai::uint128_t>::max () - system.nodes [0]->config.receive_minimum.number (), rai::test_genesis_key.prv, rai::test_genesis_key.pub, system.work.generate (genesis.hash ()));
    rai::send_block send2 (send1.hash (), key2.pub, std::numeric_limits <rai::uint128_t>::max () - system.nodes [0]->config.receive_minimum.number () * 2, rai::test_genesis_key.prv, rai::test_genesis_key.pub, system.work.generate (send1.hash ()));
    system.nodes [0]->process_receive_republish (std::unique_ptr <rai::block> (new rai::send_block (send2)), 0);
    system.nodes [0]->process_receive_republish (std::unique_ptr <rai::block> (new rai::send_block (send1)), 0);
    auto iterations (0);
    while (std::any_of (system.nodes.begin (), system.nodes.end (), [&] (std::shared_ptr <rai::node> const & node_a) {return node_a->balance (rai::test_genesis_key.pub) != rai::genesis_amount - system.nodes [0]->config.receive_minimum.number () * 2;}))
    {
        system.poll ();
        ++iterations;
        ASSERT_LT (iterations, 200);
    }
}

TEST (node, quick_confirm)
{
    rai::system system (24000, 1);
    rai::keypair key;
	rai::block_hash previous (system.nodes [0]->latest (rai::test_genesis_key.pub));
	system.wallet (0)->insert_adhoc (key.prv);
    rai::send_block send (previous, key.pub, 0, rai::test_genesis_key.prv, rai::test_genesis_key.pub, system.work.generate (previous));
	system.nodes [0]->process_receive_republish (send.clone (), 0);
    auto iterations (0);
    while (system.nodes [0]->balance (key.pub).is_zero ())
    {
        system.poll ();
        ++iterations;
        ASSERT_LT (iterations, 200);
    }
}

TEST (node, auto_bootstrap)
{
	rai::system system (24000, 1);
	rai::keypair key2;
	system.wallet (0)->insert_adhoc (rai::test_genesis_key.prv);
	system.wallet (0)->insert_adhoc (key2.prv);
	ASSERT_NE (nullptr, system.wallet (0)->send_action (rai::test_genesis_key.pub, key2.pub, system.nodes [0]->config.receive_minimum.number ()));
	auto iterations1 (0);
	while (system.nodes [0]->balance (key2.pub) != system.nodes [0]->config.receive_minimum.number ())
	{
		system.poll ();
		++iterations1;
		ASSERT_LT (iterations1, 200);
	}
	rai::node_init init1;
	auto node1 (std::make_shared <rai::node> (init1, system.service, 24001, rai::unique_path (), system.alarm, system.logging, system.work));
	ASSERT_FALSE (init1.error ());
	node1->network.send_keepalive (system.nodes [0]->network.endpoint ());
	node1->start ();
	ASSERT_EQ (0, node1->bootstrap_initiator.warmed_up);
	ASSERT_FALSE (node1->bootstrap_initiator.in_progress ());
	ASSERT_EQ (0, system.nodes [0]->bootstrap_initiator.warmed_up);
	ASSERT_FALSE (system.nodes [0]->bootstrap_initiator.in_progress ());
	auto iterations2 (0);
	while (!node1->bootstrap_initiator.in_progress () || !system.nodes [0]->bootstrap_initiator.in_progress ())
	{
		system.poll ();
		++iterations2;
		ASSERT_LT (iterations2, 200);
	}
	ASSERT_EQ (1, node1->bootstrap_initiator.warmed_up);
	ASSERT_EQ (1, system.nodes [0]->bootstrap_initiator.warmed_up);
	auto iterations3 (0);
	while (node1->balance (key2.pub) != system.nodes [0]->config.receive_minimum.number ())
	{
		system.poll ();
		++iterations3;
		ASSERT_LT (iterations3, 200);
	}
	auto iterations4 (0);
	while (node1->bootstrap_initiator.in_progress () || system.nodes [0]->bootstrap_initiator.in_progress ())
	{
		system.poll ();
		++iterations4;
		ASSERT_LT (iterations4, 200);
	};
	node1->stop ();
}

TEST (node, auto_bootstrap_reverse)
{
    rai::system system (24000, 1);
    rai::keypair key2;
	system.wallet (0)->insert_adhoc (rai::test_genesis_key.prv);
	system.wallet (0)->insert_adhoc (key2.prv);
    rai::node_init init1;
    auto node1 (std::make_shared <rai::node> (init1, system.service, 24001, rai::unique_path (), system.alarm, system.logging, system.work));
    ASSERT_FALSE (init1.error ());
    ASSERT_NE (nullptr, system.wallet (0)->send_action (rai::test_genesis_key.pub, key2.pub, system.nodes [0]->config.receive_minimum.number ()));
    system.nodes [0]->network.send_keepalive (node1->network.endpoint ());
    node1->start ();
    auto iterations (0);
    while (node1->balance (key2.pub) != system.nodes [0]->config.receive_minimum.number ())
    {
        system.poll ();
        ++iterations;
        ASSERT_LT (iterations, 200);
    }
    node1->stop ();
}

TEST (node, receive_gap)
{
    rai::system system (24000, 1);
    auto & node1 (*system.nodes [0]);
    ASSERT_EQ (0, node1.gap_cache.blocks.size ());
    rai::send_block block (0, 1, 2, rai::keypair ().prv, 4, 5);
    rai::confirm_req message;
    message.block = block.clone ();
    node1.process_message (message, node1.network.endpoint ());
    ASSERT_EQ (1, node1.gap_cache.blocks.size ());
}

TEST (node, merge_peers)
{
	rai::system system (24000, 1);
	std::array <rai::endpoint, 8> endpoints;
	endpoints.fill (rai::endpoint (boost::asio::ip::address_v6::loopback (), 24000));
	endpoints [0] = rai::endpoint (boost::asio::ip::address_v6::loopback (), 24001);
	system.nodes [0]->network.merge_peers (endpoints);
	ASSERT_EQ (0, system.nodes [0]->peers.peers.size ());
}

TEST (node, search_pending)
{
    rai::system system (24000, 1);
    rai::keypair key2;
	system.wallet (0)->insert_adhoc (rai::test_genesis_key.prv);
    ASSERT_NE (nullptr, system.wallet (0)->send_action (rai::test_genesis_key.pub, key2.pub, system.nodes [0]->config.receive_minimum.number ()));
    system.wallet (0)->insert_adhoc (key2.prv);
	auto node (system.nodes [0]);
	ASSERT_FALSE (system.wallet (0)->search_pending ());
    auto iterations2 (0);
    while (system.nodes [0]->balance (key2.pub).is_zero ())
    {
        system.poll ();
        ++iterations2;
        ASSERT_LT (iterations2, 200);
    }
}

TEST (node, search_pending_same)
{
    rai::system system (24000, 1);
    rai::keypair key2;
	system.wallet (0)->insert_adhoc (rai::test_genesis_key.prv);
    ASSERT_NE (nullptr, system.wallet (0)->send_action (rai::test_genesis_key.pub, key2.pub, system.nodes [0]->config.receive_minimum.number ()));
    ASSERT_NE (nullptr, system.wallet (0)->send_action (rai::test_genesis_key.pub, key2.pub, system.nodes [0]->config.receive_minimum.number ()));
    system.wallet (0)->insert_adhoc (key2.prv);
	auto node (system.nodes [0]);
	ASSERT_FALSE (system.wallet (0)->search_pending ());
    auto iterations2 (0);
    while (system.nodes [0]->balance (key2.pub) != 2 * system.nodes [0]->config.receive_minimum.number ())
    {
        system.poll ();
        ++iterations2;
        ASSERT_LT (iterations2, 200);
    }
}

TEST (node, search_pending_multiple)
{
    rai::system system (24000, 1);
    rai::keypair key2;
	rai::keypair key3;
	system.wallet (0)->insert_adhoc (rai::test_genesis_key.prv);
	system.wallet (0)->insert_adhoc (key3.prv);
    ASSERT_NE (nullptr, system.wallet (0)->send_action (rai::test_genesis_key.pub, key3.pub, system.nodes [0]->config.receive_minimum.number ()));
	auto iterations1 (0);
    while (system.nodes [0]->balance (key3.pub).is_zero ())
    {
        system.poll ();
        ++iterations1;
        ASSERT_LT (iterations1, 200);
    }
    ASSERT_NE (nullptr, system.wallet (0)->send_action (rai::test_genesis_key.pub, key2.pub, system.nodes [0]->config.receive_minimum.number ()));
    ASSERT_NE (nullptr, system.wallet (0)->send_action (key3.pub, key2.pub, system.nodes [0]->config.receive_minimum.number ()));
    system.wallet (0)->insert_adhoc (key2.prv);
	auto node (system.nodes [0]);
	ASSERT_FALSE (system.wallet (0)->search_pending ());
    auto iterations2 (0);
    while (system.nodes [0]->balance (key2.pub) != 2 * system.nodes [0]->config.receive_minimum.number ())
    {
        system.poll ();
        ++iterations2;
        ASSERT_LT (iterations2, 200);
    }
}

TEST (node, unlock_search)
{
    rai::system system (24000, 1);
    rai::keypair key2;
	rai::uint128_t balance (system.nodes [0]->balance (rai::test_genesis_key.pub));
	{
		rai::transaction transaction (system.wallet (0)->store.environment, nullptr, true);
		system.wallet (0)->store.rekey (transaction, "");
	}
	system.wallet (0)->insert_adhoc (rai::test_genesis_key.prv);
    ASSERT_NE (nullptr, system.wallet (0)->send_action (rai::test_genesis_key.pub, key2.pub, system.nodes [0]->config.receive_minimum.number ()));
    auto iterations1 (0);
    while (system.nodes [0]->balance (rai::test_genesis_key.pub) == balance)
    {
        system.poll ();
        ++iterations1;
        ASSERT_LT (iterations1, 200);
    }
	system.wallet (0)->insert_adhoc (key2.prv);
	system.wallet (0)->store.password.value_set (rai::keypair ().prv);
	auto node (system.nodes [0]);
	ASSERT_FALSE (system.wallet (0)->enter_password (""));
    auto iterations2 (0);
    while (system.nodes [0]->balance (key2.pub).is_zero ())
    {
        system.poll ();
        ++iterations2;
        ASSERT_LT (iterations2, 200);
    }
}

TEST (node, connect_after_junk)
{
    rai::system system (24000, 1);
    rai::node_init init1;
    auto node1 (std::make_shared <rai::node> (init1, system.service, 24001, rai::unique_path (), system.alarm, system.logging, system.work));
    uint64_t junk (0);
    node1->network.socket.async_send_to (boost::asio::buffer (&junk, sizeof (junk)), system.nodes [0]->network.endpoint (), [] (boost::system::error_code const &, size_t) {});
    auto iterations1 (0);
    while (system.nodes [0]->network.error_count == 0)
    {
        system.poll ();
        ++iterations1;
        ASSERT_LT (iterations1, 200);
    }
    node1->start ();
    node1->network.send_keepalive (system.nodes [0]->network.endpoint ());
    auto iterations2 (0);
    while (node1->peers.empty ())
    {
        system.poll ();
        ++iterations2;
        ASSERT_LT (iterations2, 200);
    }
    node1->stop ();
}

TEST (node, working)
{
	auto path (rai::working_path ());
	ASSERT_FALSE (path.empty ());
}

TEST (logging, serialization)
{
	auto path (rai::unique_path ());
	rai::logging logging1 (path);
	logging1.ledger_logging_value = !logging1.ledger_logging_value;
	logging1.ledger_duplicate_logging_value = !logging1.ledger_duplicate_logging_value;
	logging1.network_logging_value = !logging1.network_logging_value;
	logging1.network_message_logging_value = !logging1.network_message_logging_value;
	logging1.network_publish_logging_value = !logging1.network_publish_logging_value;
	logging1.network_packet_logging_value = !logging1.network_packet_logging_value;
	logging1.network_keepalive_logging_value = !logging1.network_keepalive_logging_value;
	logging1.node_lifetime_tracing_value = !logging1.node_lifetime_tracing_value;
	logging1.insufficient_work_logging_value = !logging1.insufficient_work_logging_value;
	logging1.log_rpc_value = !logging1.log_rpc_value;
	logging1.bulk_pull_logging_value = !logging1.bulk_pull_logging_value;
	logging1.work_generation_time_value = !logging1.work_generation_time_value;
	logging1.log_to_cerr_value = !logging1.log_to_cerr_value;
	logging1.max_size = 10;
	boost::property_tree::ptree tree;
	logging1.serialize_json (tree);
	rai::logging logging2 (path);
	bool upgraded (false);
	ASSERT_FALSE (logging2.deserialize_json (upgraded, tree));
	ASSERT_FALSE (upgraded);
	ASSERT_EQ (logging1.ledger_logging_value, logging2.ledger_logging_value);
	ASSERT_EQ (logging1.ledger_duplicate_logging_value, logging2.ledger_duplicate_logging_value);
	ASSERT_EQ (logging1.network_logging_value, logging2.network_logging_value);
	ASSERT_EQ (logging1.network_message_logging_value, logging2.network_message_logging_value);
	ASSERT_EQ (logging1.network_publish_logging_value, logging2.network_publish_logging_value);
	ASSERT_EQ (logging1.network_packet_logging_value, logging2.network_packet_logging_value);
	ASSERT_EQ (logging1.network_keepalive_logging_value, logging2.network_keepalive_logging_value);
	ASSERT_EQ (logging1.node_lifetime_tracing_value, logging2.node_lifetime_tracing_value);
	ASSERT_EQ (logging1.insufficient_work_logging_value, logging2.insufficient_work_logging_value);
	ASSERT_EQ (logging1.log_rpc_value, logging2.log_rpc_value);
	ASSERT_EQ (logging1.bulk_pull_logging_value, logging2.bulk_pull_logging_value);
	ASSERT_EQ (logging1.work_generation_time_value, logging2.work_generation_time_value);
	ASSERT_EQ (logging1.log_to_cerr_value, logging2.log_to_cerr_value);
	ASSERT_EQ (logging1.max_size, logging2.max_size);
}

TEST (logging, upgrade_v1_v2)
{
	auto path1 (rai::unique_path ());
	auto path2 (rai::unique_path ());
	rai::logging logging1 (path1);
	rai::logging logging2 (path2);
	boost::property_tree::ptree tree;
	logging1.serialize_json (tree);
	tree.erase ("version");
	tree.erase ("vote");
	bool upgraded (false);
	ASSERT_FALSE (logging2.deserialize_json (upgraded, tree));
	ASSERT_EQ ("2", tree.get <std::string> ("version"));
	ASSERT_EQ (false, tree.get <bool> ("vote"));
}

TEST (node, price)
{
	rai::system system (24000, 1);
	auto price1 (system.nodes [0]->price (rai::Grai_ratio, 1));
	ASSERT_EQ (rai::node::price_max * 100.0, price1);
	auto price2 (system.nodes [0]->price (rai::Grai_ratio * int (rai::node::free_cutoff + 1), 1));
	ASSERT_EQ (0, price2);
	auto price3 (system.nodes [0]->price (rai::Grai_ratio * int (rai::node::free_cutoff + 2) / 2, 1));
	ASSERT_EQ (rai::node::price_max * 100.0 / 2, price3);
	auto price4 (system.nodes [0]->price (rai::Grai_ratio * int (rai::node::free_cutoff) * 2, 1));
	ASSERT_EQ (0, price4);
}

TEST (node_config, serialization)
{
	auto path (rai::unique_path ());
	rai::logging logging1 (path);
	rai::node_config config1 (100, logging1);
	config1.packet_delay_microseconds = 10;
	config1.bootstrap_fraction_numerator = 10;
	config1.creation_rebroadcast = 10;
	config1.rebroadcast_delay = 10;
	config1.receive_minimum = 10;
	config1.inactive_supply = 10;
	config1.password_fanout = 10;
	boost::property_tree::ptree tree;
	config1.serialize_json (tree);
	rai::logging logging2 (path);
	logging2.node_lifetime_tracing_value = !logging2.node_lifetime_tracing_value;
	rai::node_config config2 (50, logging2);
	ASSERT_NE (config2.packet_delay_microseconds, config1.packet_delay_microseconds);
	ASSERT_NE (config2.bootstrap_fraction_numerator, config1.bootstrap_fraction_numerator);
	ASSERT_NE (config2.creation_rebroadcast, config1.creation_rebroadcast);
	ASSERT_NE (config2.rebroadcast_delay, config1.rebroadcast_delay);
	ASSERT_NE (config2.peering_port, config1.peering_port);
	ASSERT_NE (config2.logging.node_lifetime_tracing_value, config1.logging.node_lifetime_tracing_value);
	ASSERT_NE (config2.inactive_supply, config1.inactive_supply);
	ASSERT_NE (config2.password_fanout, config1.password_fanout);
	bool upgraded (false);
	config2.deserialize_json (upgraded, tree);
	ASSERT_FALSE (upgraded);
	ASSERT_EQ (config2.packet_delay_microseconds, config1.packet_delay_microseconds);
	ASSERT_EQ (config2.bootstrap_fraction_numerator, config1.bootstrap_fraction_numerator);
	ASSERT_EQ (config2.creation_rebroadcast, config1.creation_rebroadcast);
	ASSERT_EQ (config2.rebroadcast_delay, config1.rebroadcast_delay);
	ASSERT_EQ (config2.peering_port, config1.peering_port);
	ASSERT_EQ (config2.logging.node_lifetime_tracing_value, config1.logging.node_lifetime_tracing_value);
	ASSERT_EQ (config2.inactive_supply, config1.inactive_supply);
	ASSERT_EQ (config2.password_fanout, config1.password_fanout);
}

TEST (node_config, v1_v2_upgrade)
{
	auto path (rai::unique_path ());
	rai::logging logging1 (path);
	boost::property_tree::ptree tree;
	tree.put ("peering_port", std::to_string (0));
	tree.put ("packet_delay_microseconds", std::to_string (0));
	tree.put ("bootstrap_fraction_numerator", std::to_string (0));
	tree.put ("creation_rebroadcast", std::to_string (0));
	tree.put ("rebroadcast_delay", std::to_string (0));
	tree.put ("receive_minimum", rai::amount (0).to_string_dec ());
	boost::property_tree::ptree logging_l;
	logging1.serialize_json (logging_l);
	tree.add_child ("logging", logging_l);
	boost::property_tree::ptree preconfigured_peers_l;
	tree.add_child ("preconfigured_peers", preconfigured_peers_l);
	boost::property_tree::ptree preconfigured_representatives_l;
	tree.add_child ("preconfigured_representatives", preconfigured_representatives_l);
	bool upgraded (false);
	rai::node_config config1 (path);
	ASSERT_FALSE (tree.get_child_optional ("work_peers"));
	config1.deserialize_json (upgraded, tree);
	ASSERT_TRUE (upgraded);
	ASSERT_TRUE (!!tree.get_child_optional ("work_peers"));
}

TEST (node_config, unversioned_v2_upgrade)
{
	auto path (rai::unique_path ());
	rai::logging logging1 (path);
	boost::property_tree::ptree tree;
	tree.put ("peering_port", std::to_string (0));
	tree.put ("packet_delay_microseconds", std::to_string (0));
	tree.put ("bootstrap_fraction_numerator", std::to_string (0));
	tree.put ("creation_rebroadcast", std::to_string (0));
	tree.put ("rebroadcast_delay", std::to_string (0));
	tree.put ("receive_minimum", rai::amount (0).to_string_dec ());
	boost::property_tree::ptree logging_l;
	logging1.serialize_json (logging_l);
	tree.add_child ("logging", logging_l);
	boost::property_tree::ptree preconfigured_peers_l;
	tree.add_child ("preconfigured_peers", preconfigured_peers_l);
	boost::property_tree::ptree preconfigured_representatives_l;
	boost::property_tree::ptree entry;
	entry.put ("", "TR6ZJ4pdp6HC76xMRpVDny5x2s8AEbrhFue3NKVxYYdmKuTEib");
	preconfigured_representatives_l.push_back (std::make_pair ("", entry));
	tree.add_child ("preconfigured_representatives", preconfigured_representatives_l);
	boost::property_tree::ptree work_peers_l;
	tree.add_child ("work_peers", work_peers_l);
	bool upgraded (false);
	rai::node_config config1 (path);
	ASSERT_FALSE (tree.get_optional <std::string> ("version"));
	config1.deserialize_json (upgraded, tree);
	ASSERT_TRUE (upgraded);
	ASSERT_EQ (1, config1.preconfigured_representatives.size ());
	ASSERT_EQ ("xrb_3e3j5tkog48pnny9dmfzj1r16pg8t1e76dz5tmac6iq689wyjfpiij4txtdo", config1.preconfigured_representatives [0].to_account ());
	auto reps (tree.get_child ("preconfigured_representatives"));
	ASSERT_EQ (1, reps.size ());
	ASSERT_EQ ("xrb_3e3j5tkog48pnny9dmfzj1r16pg8t1e76dz5tmac6iq689wyjfpiij4txtdo", reps.begin ()->second.get <std::string> (""));
	auto version (tree.get <std::string> ("version"));
	ASSERT_GT (std::stoull (version), 1);
}

TEST (node_config, v2_v3_upgrade)
{
	auto path (rai::unique_path ());
	rai::logging logging1 (path);
	boost::property_tree::ptree tree;
	tree.put ("peering_port", std::to_string (0));
	tree.put ("packet_delay_microseconds", std::to_string (0));
	tree.put ("bootstrap_fraction_numerator", std::to_string (0));
	tree.put ("creation_rebroadcast", std::to_string (0));
	tree.put ("rebroadcast_delay", std::to_string (0));
	tree.put ("receive_minimum", rai::amount (0).to_string_dec ());
	tree.put ("version", "2");
	boost::property_tree::ptree logging_l;
	logging1.serialize_json (logging_l);
	tree.add_child ("logging", logging_l);
	boost::property_tree::ptree preconfigured_peers_l;
	tree.add_child ("preconfigured_peers", preconfigured_peers_l);
	boost::property_tree::ptree preconfigured_representatives_l;
	boost::property_tree::ptree entry;
	entry.put ("", "TR6ZJ4pdp6HC76xMRpVDny5x2s8AEbrhFue3NKVxYYdmKuTEib");
	preconfigured_representatives_l.push_back (std::make_pair ("", entry));
	tree.add_child ("preconfigured_representatives", preconfigured_representatives_l);
	boost::property_tree::ptree work_peers_l;
	tree.add_child ("work_peers", work_peers_l);
	bool upgraded (false);
	rai::node_config config1 (path);
	ASSERT_FALSE (tree.get_optional <std::string> ("inactive_supply"));
	ASSERT_FALSE (tree.get_optional <std::string> ("password_fanout"));
	ASSERT_FALSE (tree.get_optional <std::string> ("io_threads"));
	ASSERT_FALSE (tree.get_optional <std::string> ("work_threads"));
	config1.deserialize_json (upgraded, tree);
	ASSERT_EQ (rai::uint128_union (0).to_string_dec (), tree.get <std::string> ("inactive_supply"));
	ASSERT_EQ ("1024", tree.get <std::string> ("password_fanout"));
	ASSERT_NE (0, std::stoul (tree.get <std::string> ("password_fanout")));
	ASSERT_NE (0, std::stoul (tree.get <std::string> ("password_fanout")));
	ASSERT_TRUE (upgraded);
	auto version (tree.get <std::string> ("version"));
	ASSERT_GT (std::stoull (version), 2);
}

TEST (node, confirm_locked)
{
	rai::system system (24000, 1);
	system.wallet (0)->insert_adhoc (rai::test_genesis_key.prv);
	system.wallet (0)->enter_password ("1");
	rai::send_block block (0, 0, 0, rai::keypair ().prv, 0, 0);
	system.nodes [0]->process_confirmation (block, rai::endpoint ());
}

TEST (node_config, random_rep)
{
	auto path (rai::unique_path ());
	rai::logging logging1 (path);
	rai::node_config config1 (100, logging1);
	auto rep (config1.random_representative ());
	ASSERT_NE (config1.preconfigured_representatives.end (), std::find (config1.preconfigured_representatives.begin (), config1.preconfigured_representatives.end (), rep));
}

TEST (node, block_replace)
{
	rai::system system (24000, 2);
	system.wallet (0)->insert_adhoc (rai::test_genesis_key.prv);
    ASSERT_NE (nullptr, system.wallet (0)->send_action (rai::test_genesis_key.pub, 0, 1000));
	std::unique_ptr <rai::block> block1;
	{
		rai::transaction transaction (system.nodes [0]->store.environment, nullptr, false);
		block1 = system.nodes [0]->store.block_get (transaction, system.nodes [0]->ledger.latest (transaction, rai::test_genesis_key.pub));
	}
	ASSERT_NE (nullptr, block1);
	auto initial_work (block1->block_work ());
	while (system.work.work_value (block1->root (), block1->block_work ()) <= system.work.work_value (block1->root (), initial_work))
	{
		system.nodes [1]->generate_work (*block1);
	}
	system.nodes [1]->network.republish_block (*block1, 0);
	auto iterations1 (0);
	std::unique_ptr <rai::block> block2;
	while (block2 == nullptr)
	{
		system.poll ();
		++iterations1;
		ASSERT_LT (iterations1, 200);
		rai::transaction transaction (system.nodes [0]->store.environment, nullptr, false);
		auto block (system.nodes [0]->store.block_get (transaction, system.nodes [0]->ledger.latest (transaction, rai::test_genesis_key.pub)));
		if (block->block_work () != initial_work)
		{
			block2 = std::move (block);
		}
	}
	ASSERT_NE (initial_work, block1->block_work ());
	ASSERT_EQ (block1->block_work (), block2->block_work ());
	ASSERT_GT (system.work.work_value (block2->root (), block2->block_work ()), system.work.work_value (block1->root (), initial_work));
}

TEST (node, fork_publish)
{
    std::weak_ptr <rai::node> node0;
    {
        rai::system system (24000, 1);
        node0 = system.nodes [0];
        auto & node1 (*system.nodes [0]);
		system.wallet (0)->insert_adhoc (rai::test_genesis_key.prv);
        rai::keypair key1;
		rai::genesis genesis;
        rai::send_block send1 (genesis.hash (), key1.pub, rai::genesis_amount - 100, rai::test_genesis_key.prv, rai::test_genesis_key.pub, 0);
        rai::keypair key2;
        rai::send_block send2 (genesis.hash (), key2.pub, rai::genesis_amount - 100, rai::test_genesis_key.prv, rai::test_genesis_key.pub, 0);
        node1.process_receive_republish (send1.clone (), 0);
        ASSERT_EQ (1, node1.active.roots.size ());
		auto existing (node1.active.roots.find (send1.root ()));
		ASSERT_NE (node1.active.roots.end (), existing);
		auto election (existing->election);
		ASSERT_EQ (2, election->votes.rep_votes.size ());
        node1.process_receive_republish (send2.clone (), 0);
        auto existing1 (election->votes.rep_votes.find (rai::test_genesis_key.pub));
        ASSERT_NE (election->votes.rep_votes.end (), existing1);
        ASSERT_EQ (send1, *existing1->second);
		rai::transaction transaction (node1.store.environment, nullptr, false);
        auto winner (node1.ledger.winner (transaction, election->votes));
        ASSERT_EQ (send1, *winner.second);
        ASSERT_EQ (rai::genesis_amount - 100, winner.first);
    }
    ASSERT_TRUE (node0.expired ());
}

TEST (node, fork_keep)
{
    rai::system system (24000, 2);
    auto & node1 (*system.nodes [0]);
    auto & node2 (*system.nodes [1]);
	ASSERT_EQ (1, node1.peers.size ());
	system.wallet (0)->insert_adhoc (rai::test_genesis_key.prv);
    rai::keypair key1;
	rai::genesis genesis;
    std::unique_ptr <rai::send_block> send1 (new rai::send_block (genesis.hash (), key1.pub, rai::genesis_amount - 100, rai::test_genesis_key.prv, rai::test_genesis_key.pub, system.work.generate (genesis.hash ())));
    rai::publish publish1;
    publish1.block = std::move (send1);
    rai::keypair key2;
    std::unique_ptr <rai::send_block> send2 (new rai::send_block (genesis.hash (), key2.pub, rai::genesis_amount - 100, rai::test_genesis_key.prv, rai::test_genesis_key.pub, system.work.generate (genesis.hash ())));
    rai::publish publish2;
    publish2.block = std::move (send2);
    node1.process_message (publish1, node1.network.endpoint ());
	node2.process_message (publish1, node2.network.endpoint ());
    ASSERT_EQ (1, node1.active.roots.size ());
    ASSERT_EQ (1, node2.active.roots.size ());
    node1.process_message (publish2, node1.network.endpoint ());
	node2.process_message (publish2, node2.network.endpoint ());
    auto conflict (node2.active.roots.find (genesis.hash ()));
    ASSERT_NE (node2.active.roots.end (), conflict);
    auto votes1 (conflict->election);
    ASSERT_NE (nullptr, votes1);
    ASSERT_EQ (1, votes1->votes.rep_votes.size ());
	{
		rai::transaction transaction (system.nodes [0]->store.environment, nullptr, false);
		ASSERT_TRUE (system.nodes [0]->store.block_exists (transaction, publish1.block->hash ()));
		ASSERT_TRUE (system.nodes [1]->store.block_exists (transaction, publish1.block->hash ()));
	}
    auto iterations (0);
    while (votes1->votes.rep_votes.size () == 1)
	{
		system.poll ();
        ++iterations;
        ASSERT_LT (iterations, 2000);
	}
	rai::transaction transaction (system.nodes [0]->store.environment, nullptr, false);
    auto winner (node1.ledger.winner (transaction, votes1->votes));
    ASSERT_EQ (*publish1.block, *winner.second);
    ASSERT_EQ (rai::genesis_amount - 100, winner.first);
	ASSERT_TRUE (system.nodes [0]->store.block_exists (transaction, publish1.block->hash ()));
	ASSERT_TRUE (system.nodes [1]->store.block_exists (transaction, publish1.block->hash ()));
}

TEST (node, fork_flip)
{
    rai::system system (24000, 2);
    auto & node1 (*system.nodes [0]);
    auto & node2 (*system.nodes [1]);
    ASSERT_EQ (1, node1.peers.size ());
	system.wallet (0)->insert_adhoc (rai::test_genesis_key.prv);
    rai::keypair key1;
	rai::genesis genesis;
    std::unique_ptr <rai::send_block> send1 (new rai::send_block (genesis.hash (), key1.pub, rai::genesis_amount - 100, rai::test_genesis_key.prv, rai::test_genesis_key.pub, system.work.generate (genesis.hash ())));
    rai::publish publish1;
    publish1.block = std::move (send1);
    rai::keypair key2;
    std::unique_ptr <rai::send_block> send2 (new rai::send_block (genesis.hash (), key2.pub, rai::genesis_amount - 100, rai::test_genesis_key.prv, rai::test_genesis_key.pub, system.work.generate (genesis.hash ())));
    rai::publish publish2;
    publish2.block = std::move (send2);
    node1.process_message (publish1, node1.network.endpoint ());
    node2.process_message (publish2, node1.network.endpoint ());
    ASSERT_EQ (1, node1.active.roots.size ());
    ASSERT_EQ (1, node2.active.roots.size ());
    node1.process_message (publish2, node1.network.endpoint ());
    node2.process_message (publish1, node2.network.endpoint ());
    auto conflict (node2.active.roots.find (genesis.hash ()));
    ASSERT_NE (node2.active.roots.end (), conflict);
    auto votes1 (conflict->election);
    ASSERT_NE (nullptr, votes1);
    ASSERT_EQ (1, votes1->votes.rep_votes.size ());
	{
		rai::transaction transaction (system.nodes [0]->store.environment, nullptr, false);
		ASSERT_TRUE (node1.store.block_exists (transaction, publish1.block->hash ()));
	}
	{
		rai::transaction transaction (system.nodes [1]->store.environment, nullptr, false);
		ASSERT_TRUE (node2.store.block_exists (transaction, publish2.block->hash ()));
	}
    auto iterations (0);
    while (votes1->votes.rep_votes.size () == 1)
    {
        system.poll ();
        ++iterations;
        ASSERT_LT (iterations, 200);
    }
	rai::transaction transaction (system.nodes [0]->store.environment, nullptr, false);
    auto winner (node2.ledger.winner (transaction, votes1->votes));
    ASSERT_EQ (*publish1.block, *winner.second);
    ASSERT_EQ (rai::genesis_amount - 100, winner.first);
    ASSERT_TRUE (node1.store.block_exists (transaction, publish1.block->hash ()));
    ASSERT_TRUE (node2.store.block_exists (transaction, publish1.block->hash ()));
    ASSERT_FALSE (node2.store.block_exists (transaction, publish2.block->hash ()));
}

TEST (node, fork_multi_flip)
{
    rai::system system (24000, 2);
    auto & node1 (*system.nodes [0]);
    auto & node2 (*system.nodes [1]);
	ASSERT_EQ (1, node1.peers.size ());
	system.wallet (0)->insert_adhoc (rai::test_genesis_key.prv);
    rai::keypair key1;
	rai::genesis genesis;
    std::unique_ptr <rai::send_block> send1 (new rai::send_block (genesis.hash (), key1.pub, rai::genesis_amount - 100, rai::test_genesis_key.prv, rai::test_genesis_key.pub, system.work.generate (genesis.hash ())));
    rai::publish publish1;
    publish1.block = std::move (send1);
    rai::keypair key2;
    std::unique_ptr <rai::send_block> send2 (new rai::send_block (genesis.hash (), key2.pub, rai::genesis_amount - 100, rai::test_genesis_key.prv, rai::test_genesis_key.pub, system.work.generate (genesis.hash ())));
    rai::publish publish2;
    publish2.block = std::move (send2);
    std::unique_ptr <rai::send_block> send3 (new rai::send_block (publish2.block->hash (), key2.pub, rai::genesis_amount - 100, rai::test_genesis_key.prv, rai::test_genesis_key.pub, system.work.generate (publish2.block->hash ())));
    rai::publish publish3;
    publish3.block = std::move (send3);
    node1.process_message (publish1, node1.network.endpoint ());
	node2.process_message (publish2, node2.network.endpoint ());
    node2.process_message (publish3, node2.network.endpoint ());
    ASSERT_EQ (1, node1.active.roots.size ());
    ASSERT_EQ (2, node2.active.roots.size ());
    node1.process_message (publish2, node1.network.endpoint ());
    node1.process_message (publish3, node1.network.endpoint ());
	node2.process_message (publish1, node2.network.endpoint ());
    auto conflict (node2.active.roots.find (genesis.hash ()));
    ASSERT_NE (node2.active.roots.end (), conflict);
    auto votes1 (conflict->election);
    ASSERT_NE (nullptr, votes1);
    ASSERT_EQ (1, votes1->votes.rep_votes.size ());
	{
		rai::transaction transaction (system.nodes [0]->store.environment, nullptr, false);
		ASSERT_TRUE (node1.store.block_exists (transaction, publish1.block->hash ()));
	}
	{
		rai::transaction transaction (system.nodes [1]->store.environment, nullptr, false);
		ASSERT_TRUE (node2.store.block_exists (transaction, publish2.block->hash ()));
		ASSERT_TRUE (node2.store.block_exists (transaction, publish3.block->hash ()));
	}
    auto iterations (0);
    while (votes1->votes.rep_votes.size () == 1)
	{
		system.poll ();
        ++iterations;
        ASSERT_LT (iterations, 200);
	}
	rai::transaction transaction (system.nodes [0]->store.environment, nullptr, false);
    auto winner (node1.ledger.winner (transaction, votes1->votes));
    ASSERT_EQ (*publish1.block, *winner.second);
    ASSERT_EQ (rai::genesis_amount - 100, winner.first);
	ASSERT_TRUE (node1.store.block_exists (transaction, publish1.block->hash ()));
	ASSERT_TRUE (node2.store.block_exists (transaction, publish1.block->hash ()));
	ASSERT_FALSE (node2.store.block_exists (transaction, publish2.block->hash ()));
    ASSERT_FALSE (node2.store.block_exists (transaction, publish3.block->hash ()));
}

// Blocks that are no longer actively being voted on should be able to be evicted through bootstrapping.
// This could happen if a fork wasn't resolved before the process previously shut down
TEST (node, fork_bootstrap_flip)
{
	rai::system system0 (24000, 1);
	rai::system system1 (24001, 1);
	auto & node1 (*system0.nodes [0]);
	auto & node2 (*system1.nodes [0]);
	system0.wallet (0)->insert_adhoc (rai::test_genesis_key.prv);
	rai::block_hash latest (system0.nodes [0]->latest (rai::test_genesis_key.pub));
	rai::keypair key1;
	rai::send_block send1 (latest, key1.pub, rai::genesis_amount - 100, rai::test_genesis_key.prv, rai::test_genesis_key.pub, system0.work.generate (latest));
	rai::keypair key2;
	rai::send_block send2 (latest, key2.pub, rai::genesis_amount - 100, rai::test_genesis_key.prv, rai::test_genesis_key.pub, system0.work.generate (latest));
	// Insert but don't rebroadcast, simulating well-established blocks
	node1.process_receive_many (send1);
	node2.process_receive_many (send2);
	{
		rai::transaction transaction (node2.store.environment, nullptr, false);
		ASSERT_TRUE (node2.store.block_exists (transaction, send2.hash ()));
	}
	node1.network.send_keepalive (node2.network.endpoint ());
	auto iterations1 (0);
	while (node2.peers.empty ())
	{
		system0.poll ();
		system1.poll ();
		++iterations1;
		ASSERT_LT (iterations1, 1000);
	}
	node2.bootstrap_initiator.bootstrap (node1.network.endpoint ());
	auto again (true);
	auto iterations2 (0);
	while (again)
	{
		system0.poll ();
		system1.poll ();
		++iterations2;
		ASSERT_LT (iterations2, 200);
		rai::transaction transaction (node2.store.environment, nullptr, false);
		again = !node2.store.block_exists (transaction, send1.hash ());
	}
}

TEST (node, fork_open)
{
    rai::system system (24000, 1);
    auto & node1 (*system.nodes [0]);
	system.wallet (0)->insert_adhoc ( rai::test_genesis_key.prv);
    rai::keypair key1;
	rai::genesis genesis;
    std::unique_ptr <rai::send_block> send1 (new rai::send_block (genesis.hash (), key1.pub, 0, rai::test_genesis_key.prv, rai::test_genesis_key.pub, system.work.generate (genesis.hash ())));
    rai::publish publish1;
    publish1.block = std::move (send1);
    node1.process_message (publish1, node1.network.endpoint ());
    std::unique_ptr <rai::open_block> open1 (new rai::open_block (publish1.block->hash (), 1, key1.pub, key1.prv, key1.pub, system.work.generate (key1.pub)));
    rai::publish publish2;
    publish2.block = std::move (open1);
    node1.process_message (publish2, node1.network.endpoint ());
    std::unique_ptr <rai::open_block> open2 (new rai::open_block (publish1.block->hash (), 2, key1.pub, key1.prv, key1.pub, system.work.generate (key1.pub)));
    rai::publish publish3;
    publish3.block = std::move (open2);
    ASSERT_EQ (2, node1.active.roots.size ());
	node1.process_message (publish3, node1.network.endpoint ());
}

TEST (node, fork_open_flip)
{
    rai::system system (24000, 2);
    auto & node1 (*system.nodes [0]);
    auto & node2 (*system.nodes [1]);
    ASSERT_EQ (1, node1.peers.size ());
	system.wallet (0)->insert_adhoc (rai::test_genesis_key.prv);
    rai::keypair key1;
	rai::genesis genesis;
    std::unique_ptr <rai::send_block> send1 (new rai::send_block (genesis.hash (), key1.pub, rai::genesis_amount - 1, rai::test_genesis_key.prv, rai::test_genesis_key.pub, system.work.generate (genesis.hash ())));
    rai::publish publish1;
    publish1.block = std::move (send1);
    node1.process_message (publish1, node1.network.endpoint ());
    node2.process_message (publish1, node2.network.endpoint ());
    std::unique_ptr <rai::open_block> open1 (new rai::open_block (publish1.block->hash (), 1, key1.pub, key1.prv, key1.pub, system.work.generate (key1.pub)));
    rai::publish publish2;
    publish2.block = std::move (open1);
    std::unique_ptr <rai::open_block> open2 (new rai::open_block (publish1.block->hash (), 2, key1.pub, key1.prv, key1.pub, system.work.generate (key1.pub)));
    rai::publish publish3;
    publish3.block = std::move (open2);
    node1.process_message (publish2, node1.network.endpoint ());
    node2.process_message (publish3, node2.network.endpoint ());
    ASSERT_EQ (2, node1.active.roots.size ());
    ASSERT_EQ (2, node2.active.roots.size ());
    node1.process_message (publish3, node1.network.endpoint ());
    node2.process_message (publish2, node2.network.endpoint ());
    auto conflict (node2.active.roots.find (publish2.block->root ()));
    ASSERT_NE (node2.active.roots.end (), conflict);
    auto votes1 (conflict->election);
    ASSERT_NE (nullptr, votes1);
    ASSERT_EQ (1, votes1->votes.rep_votes.size ());
	{
		rai::transaction transaction (system.nodes [0]->store.environment, nullptr, false);
		ASSERT_TRUE (node1.store.block_exists (transaction, publish2.block->hash ()));
	}
	{
		rai::transaction transaction (system.nodes [1]->store.environment, nullptr, false);
		ASSERT_TRUE (node2.store.block_exists (transaction, publish3.block->hash ()));
	}
    auto iterations (0);
    while (votes1->votes.rep_votes.size () == 1)
    {
        system.poll ();
        ++iterations;
        ASSERT_LT (iterations, 200);
    }
	rai::transaction transaction (system.nodes [0]->store.environment, nullptr, false);
    auto winner (node2.ledger.winner (transaction, votes1->votes));
    ASSERT_EQ (*publish2.block, *winner.second);
    ASSERT_EQ (rai::genesis_amount - 1, winner.first);
    ASSERT_TRUE (node1.store.block_exists (transaction, publish2.block->hash ()));
    ASSERT_TRUE (node2.store.block_exists (transaction, publish2.block->hash ()));
    ASSERT_FALSE (node2.store.block_exists (transaction, publish3.block->hash ()));
}

TEST (node, coherent_observer)
{
    rai::system system (24000, 1);
    auto & node1 (*system.nodes [0]);
	node1.observers.blocks.add ([&node1] (rai::block const & block_a, rai::account const & account_a, rai::amount const &)
	{
		rai::transaction transaction (node1.store.environment, nullptr, false);
		ASSERT_TRUE (node1.store.block_exists (transaction, block_a.hash ()));
	});
	system.wallet (0)->insert_adhoc (rai::test_genesis_key.prv);
	rai::keypair key;
	system.wallet (0)->send_action (rai::test_genesis_key.pub, key.pub, 1);
}

TEST (node, fork_no_vote_quorum)
{
    rai::system system (24000, 3);
    auto & node1 (*system.nodes [0]);
    auto & node2 (*system.nodes [1]);
    auto & node3 (*system.nodes [2]);
	system.wallet (0)->insert_adhoc (rai::test_genesis_key.prv);
	auto key4 (system.wallet (0)->deterministic_insert ());
	system.wallet (0)->send_action (rai::test_genesis_key.pub, key4, rai::genesis_amount / 4);
	auto key1 (system.wallet (1)->deterministic_insert ());
	system.wallet (1)->store.representative_set (rai::transaction (system.wallet (1)->store.environment, nullptr, true), key1);
	auto block (system.wallet (0)->send_action (rai::test_genesis_key.pub, key1, node1.config.receive_minimum.number ()));
	ASSERT_NE (nullptr, block);
	auto iterations (0);
	while (node3.balance (key1) != node1.config.receive_minimum.number () || node2.balance (key1) != node1.config.receive_minimum.number () || node1.balance (key1) != node1.config.receive_minimum.number ())
	{
		system.poll ();
		++iterations;
		ASSERT_LT (iterations, 200);
	}
	ASSERT_EQ (node1.config.receive_minimum.number (), node1.weight (key1));
	ASSERT_EQ (node1.config.receive_minimum.number (), node2.weight (key1));
	ASSERT_EQ (node1.config.receive_minimum.number (), node3.weight (key1));
	rai::send_block send1 (block->hash (), key1, (rai::genesis_amount / 4) - (node1.config.receive_minimum.number () * 2), rai::test_genesis_key.prv, rai::test_genesis_key.pub, system.work.generate (block->hash ()));
	ASSERT_EQ (rai::process_result::progress, node1.process (send1).code);
	ASSERT_EQ (rai::process_result::progress, node2.process (send1).code);
	ASSERT_EQ (rai::process_result::progress, node3.process (send1).code);
	auto key2 (system.wallet (2)->deterministic_insert ());
	rai::send_block send2 (block->hash (), key2, (rai::genesis_amount / 4) - (node1.config.receive_minimum.number () * 2), rai::test_genesis_key.prv, rai::test_genesis_key.pub, system.work.generate (block->hash ()));
	rai::raw_key key3;
	ASSERT_FALSE (system.wallet (1)->store.fetch (rai::transaction (system.wallet (1)->store.environment, nullptr, false), key1, key3));
	node2.network.confirm_block (key3, key1, send2.clone (), 0, node3.network.endpoint (), 0);
	while (node3.network.confirm_ack_count < 3)
	{
		system.poll ();
	}
	ASSERT_TRUE (node1.latest (rai::test_genesis_key.pub) == send1.hash ());
	ASSERT_TRUE (node2.latest (rai::test_genesis_key.pub) == send1.hash ());
	ASSERT_TRUE (node3.latest (rai::test_genesis_key.pub) == send1.hash ());
}

TEST (node, stopped_rollback)
{
    rai::system system (24000, 3);
    auto & node1 (*system.nodes [0]);
    auto & node2 (*system.nodes [1]);
    auto & node3 (*system.nodes [2]);
	system.wallet (0)->insert_adhoc (rai::test_genesis_key.prv);
	auto key1 (system.wallet (0)->deterministic_insert ());
	auto amount1 (rai::genesis_amount / 4);
	auto block1 (system.wallet (0)->send_action (rai::test_genesis_key.pub, key1, amount1));
	ASSERT_NE (nullptr, block1);
	auto iterations (0);
	while (node3.balance (key1) != amount1 || node2.balance (key1) != amount1 || node1.balance (key1) != amount1 || !node1.rollback_predicate (*block1) || !node2.rollback_predicate (*block1) || !node3.rollback_predicate (*block1))
	{
		system.poll ();
		++iterations;
		ASSERT_LT (iterations, 200);
	}
	{
		rai::transaction transaction (node1.store.environment, nullptr, true);
		auto original (node1.ledger.rollback_predicate);
		node1.ledger.rollback_predicate = [] (rai::block const &) { return false; };
		ASSERT_FALSE (node1.ledger.rollback (transaction, block1->hash ()));
		ASSERT_FALSE (node1.store.block_exists (transaction, block1->hash ()));
		node1.ledger.rollback_predicate = original;
	}
	auto key2 (system.wallet (0)->deterministic_insert ());
	auto block2 (system.wallet (0)->send_action (rai::test_genesis_key.pub, key2, amount1));
	ASSERT_NE (nullptr, block2);
	auto iterations2 (0);
	while (iterations2 < 50)
	{
		system.poll ();
		++iterations2;
		ASSERT_TRUE (node2.ledger.block_exists (block1->hash ()));
		ASSERT_FALSE (node2.ledger.block_exists (block2->hash ()));
		ASSERT_TRUE (node3.ledger.block_exists (block1->hash ()));
		ASSERT_FALSE (node3.ledger.block_exists (block2->hash ()));
	}
}

TEST (node, broadcast_elected)
{
    rai::system system (24000, 3);
	auto node0 (system.nodes [0]);
	auto node1 (system.nodes [1]);
	auto node2 (system.nodes [2]);
	rai::keypair rep_big;
	rai::keypair rep_small;
	rai::keypair rep_other;
	//std::cerr << "Big: " << rep_big.pub.to_account () << std::endl;
	//std::cerr << "Small: " << rep_small.pub.to_account () << std::endl;
	//std::cerr << "Other: " << rep_other.pub.to_account () << std::endl;
	{
		rai::transaction transaction0 (node0->store.environment, nullptr, true);
		rai::transaction transaction1 (node1->store.environment, nullptr, true);
		rai::transaction transaction2 (node2->store.environment, nullptr, true);
		rai::send_block fund_big (node0->ledger.latest (transaction0, rai::test_genesis_key.pub), rep_big.pub, 500, rai::test_genesis_key.prv, rai::test_genesis_key.pub, 0);
		rai::open_block open_big (fund_big.hash (), rep_big.pub, rep_big.pub, rep_big.prv, rep_big.pub, 0);
		rai::send_block fund_small (fund_big.hash (), rep_small.pub, 250, rai::test_genesis_key.prv, rai::test_genesis_key.pub, 0);
		rai::open_block open_small (fund_small.hash (), rep_small.pub, rep_small.pub, rep_small.prv, rep_small.pub, 0);
		rai::send_block fund_other (fund_small.hash (), rep_other.pub, 125, rai::test_genesis_key.prv, rai::test_genesis_key.pub, 0);
		rai::open_block open_other (fund_other.hash (), rep_other.pub, rep_other.pub, rep_other.prv, rep_other.pub, 0);
		node0->generate_work (fund_big);
		node0->generate_work (open_big);
		node0->generate_work (fund_small);
		node0->generate_work (open_small);
		ASSERT_EQ (rai::process_result::progress, node0->ledger.process (transaction0, fund_big).code);
		ASSERT_EQ (rai::process_result::progress, node1->ledger.process (transaction1, fund_big).code);
		ASSERT_EQ (rai::process_result::progress, node2->ledger.process (transaction2, fund_big).code);
		ASSERT_EQ (rai::process_result::progress, node0->ledger.process (transaction0, open_big).code);
		ASSERT_EQ (rai::process_result::progress, node1->ledger.process (transaction1, open_big).code);
		ASSERT_EQ (rai::process_result::progress, node2->ledger.process (transaction2, open_big).code);
		ASSERT_EQ (rai::process_result::progress, node0->ledger.process (transaction0, fund_small).code);
		ASSERT_EQ (rai::process_result::progress, node1->ledger.process (transaction1, fund_small).code);
		ASSERT_EQ (rai::process_result::progress, node2->ledger.process (transaction2, fund_small).code);
		ASSERT_EQ (rai::process_result::progress, node0->ledger.process (transaction0, open_small).code);
		ASSERT_EQ (rai::process_result::progress, node1->ledger.process (transaction1, open_small).code);
		ASSERT_EQ (rai::process_result::progress, node2->ledger.process (transaction2, open_small).code);
		ASSERT_EQ (rai::process_result::progress, node0->ledger.process (transaction0, fund_other).code);
		ASSERT_EQ (rai::process_result::progress, node1->ledger.process (transaction1, fund_other).code);
		ASSERT_EQ (rai::process_result::progress, node2->ledger.process (transaction2, fund_other).code);
		ASSERT_EQ (rai::process_result::progress, node0->ledger.process (transaction0, open_other).code);
		ASSERT_EQ (rai::process_result::progress, node1->ledger.process (transaction1, open_other).code);
		ASSERT_EQ (rai::process_result::progress, node2->ledger.process (transaction2, open_other).code);
	}
	system.wallet (0)->insert_adhoc (rep_big.prv);
	system.wallet (1)->insert_adhoc (rep_small.prv);
	system.wallet (2)->insert_adhoc (rep_other.prv);
	rai::send_block fork0 (node2->latest (rai::test_genesis_key.pub), rep_small.pub, 0, rai::test_genesis_key.prv, rai::test_genesis_key.pub, 0);
	node0->generate_work (fork0);
	node0->process_receive_republish (fork0.clone (), 0);
	node1->process_receive_republish (fork0.clone (), 0);
	rai::send_block fork1 (node2->latest (rai::test_genesis_key.pub), rep_big.pub, 0, rai::test_genesis_key.prv, rai::test_genesis_key.pub, 0);
	node0->generate_work (fork1);
	system.wallet (2)->insert_adhoc (rep_small.prv);
	node2->process_receive_republish (fork1.clone (), 0);
	//std::cerr << "fork0: " << fork_hash.to_string () << std::endl;
	//std::cerr << "fork1: " << fork1.hash ().to_string () << std::endl;
	auto iterations (0);
	while (!node2->ledger.block_exists (fork0.hash ()))
	{
		system.poll ();
		ASSERT_TRUE (node0->ledger.block_exists (fork0.hash ()));
		ASSERT_TRUE (node1->ledger.block_exists (fork0.hash ()));
		++iterations;
		ASSERT_LT (iterations, 200);
	}
}

TEST (node, rep_self_vote)
{
    rai::system system (24000, 1);
	auto node0 (system.nodes [0]);
	rai::keypair rep_big;
	{
		rai::transaction transaction0 (node0->store.environment, nullptr, true);
		rai::send_block fund_big (node0->ledger.latest (transaction0, rai::test_genesis_key.pub), rep_big.pub, rai::uint128_t ("0xb0000000000000000000000000000000"), rai::test_genesis_key.prv, rai::test_genesis_key.pub, 0);
		rai::open_block open_big (fund_big.hash (), rep_big.pub, rep_big.pub, rep_big.prv, rep_big.pub, 0);
		node0->generate_work (fund_big);
		node0->generate_work (open_big);
		ASSERT_EQ (rai::process_result::progress, node0->ledger.process (transaction0, fund_big).code);
		ASSERT_EQ (rai::process_result::progress, node0->ledger.process (transaction0, open_big).code);
	}
	system.wallet (0)->insert_adhoc (rep_big.prv);
	system.wallet (0)->insert_adhoc (rai::test_genesis_key.prv);
	rai::send_block block0 (node0->latest (rai::test_genesis_key.pub), rep_big.pub, rai::uint128_t ("0x60000000000000000000000000000000"), rai::test_genesis_key.prv, rai::test_genesis_key.pub, 0);
	node0->generate_work (block0);
	ASSERT_EQ (rai::process_result::progress, node0->process (block0).code);
	auto & active (node0->active);
	{
		rai::transaction transaction (node0->store.environment, nullptr, true);
		active.start (transaction, block0, [] (rai::block &) {});
	}
	auto existing (active.roots.find (block0.root ()));
	ASSERT_NE (active.roots.end (), existing);
	auto & rep_votes (existing->election->votes.rep_votes);
	ASSERT_EQ (3, rep_votes.size ());
	ASSERT_NE (rep_votes.end (), rep_votes.find (rai::test_genesis_key.pub));
	ASSERT_NE (rep_votes.end (), rep_votes.find (rep_big.pub));
}

// Bootstrapping shouldn't republish the blocks to the network.
TEST (node, bootstrap_no_publish)
{
    rai::system system0 (24000, 1);
    rai::system system1 (24001, 1);
	auto node0 (system0.nodes [0]);
	auto node1 (system1.nodes [0]);
	rai::keypair key0;
	// node0 knows about send0 but node1 doesn't.
	rai::send_block send0 (system0.nodes [0]->latest (rai::test_genesis_key.pub), key0.pub, 500, rai::test_genesis_key.prv, rai::test_genesis_key.pub, 0);
	{
		rai::transaction transaction (node0->store.environment, nullptr, true);
		ASSERT_EQ (rai::process_result::progress, system0.nodes [0]->ledger.process (transaction, send0).code);
	}
	node1->bootstrap_initiator.bootstrap (node0->network.endpoint ());
	ASSERT_FALSE (node1->bootstrap_initiator.in_progress ());
	while (node1->bootstrap_initiator.in_progress ())
	{
		// Poll until TCP connection is established and in_progress goes true
		system0.poll ();
	}
	ASSERT_TRUE (node1->active.roots.empty ());
	while (node1->bootstrap_initiator.in_progress ())
	{
		// Poll until the TCP connection is torn down and in_progress goes false
		system0.poll ();
		system1.poll ();
		// There should never be an active transaction because the only activity is bootstrapping 1 block which shouldn't be publishing.
		ASSERT_TRUE (node1->active.roots.empty ());
	}
}

// Bootstrapping a forked open block should succeed.
TEST (node, bootstrap_fork_open)
{
    rai::system system0 (24000, 2);
	system0.wallet(0)->insert_adhoc (rai::test_genesis_key.prv);
	auto node0 (system0.nodes [0]);
	auto node1 (system0.nodes [1]);
	rai::keypair key0;
	rai::send_block send0 (system0.nodes [0]->latest (rai::test_genesis_key.pub), key0.pub, rai::genesis_amount - 500, rai::test_genesis_key.prv, rai::test_genesis_key.pub, 0);
	rai::open_block open0 (send0.hash (), 1, key0.pub, key0.prv, key0.pub, 0);
	rai::open_block open1 (send0.hash (), 2, key0.pub, key0.prv, key0.pub, 0);
	node0->generate_work (send0);
	node0->generate_work (open0);
	node0->generate_work (open1);
	{
		rai::transaction transaction0 (node0->store.environment, nullptr, true);
		rai::transaction transaction1 (node1->store.environment, nullptr, true);
		// Both know about send0
		ASSERT_EQ (rai::process_result::progress, node0->ledger.process (transaction0, send0).code);
		ASSERT_EQ (rai::process_result::progress, node1->ledger.process (transaction1, send0).code);
		// They disagree about open0/open1
		ASSERT_EQ (rai::process_result::progress, node0->ledger.process (transaction0, open0).code);
		ASSERT_EQ (rai::process_result::progress, node1->ledger.process (transaction1, open1).code);
	}
	node1->bootstrap_initiator.bootstrap (node0->network.endpoint ());
	ASSERT_FALSE (node1->bootstrap_initiator.in_progress ());
	while (node1->bootstrap_initiator.in_progress ())
	{
		// Poll until TCP connection is established and in_progress goes true
		system0.poll ();
	}
	ASSERT_TRUE (node1->active.roots.empty ());
	int iterations (0);
	while (node1->ledger.block_exists (open1.hash ()))
	{
		// Poll until the outvoted block is evicted.
		system0.poll ();
		ASSERT_LT (iterations, 200);
		++iterations;
	}
}

TEST (node, process_unchecked_keep)
{
    rai::system system (24000, 1);
	auto & node0 (*system.nodes [0]);
	rai::keypair key0;
	rai::send_block send (0, 0, 0, key0.prv, key0.pub, 0);
	{
		rai::transaction transaction (node0.store.environment, nullptr, true);
		node0.store.unchecked_put (transaction, send.hash (), send);
	}
	node0.process_unchecked ();
	rai::transaction transaction (node0.store.environment, nullptr, false);
	ASSERT_NE (node0.store.unchecked_end (), node0.store.unchecked_begin (transaction, send.hash ()));
}
