#include "eosapi.h"

chain_controller& get_db(){
    return app().get_plugin<chain_plugin>().chain();
}

wallet_manager& get_wm(){
   return app().get_plugin<wallet_plugin>().get_wallet_manager();
}

PyObject *array_create();
void array_append_string(PyObject *array_object,std::string& s);
void array_append_int(PyObject *array_object,int n);
void array_append_double(PyObject *array_object,double n);
void array_append_uint64(PyObject *arr,unsigned long long n);


inline std::vector<Name> sort_names( std::vector<Name>&& names ) {
   std::sort( names.begin(), names.end() );
   auto itr = std::unique( names.begin(), names.end() );
   names.erase( itr, names.end() );
   return names;
}

vector<uint8_t> assemble_wast( const std::string& wast ) {
   IR::Module module;
   std::vector<WAST::Error> parseErrors;
   WAST::parseModule(wast.c_str(),wast.size(),module,parseErrors);
   if(parseErrors.size())
   {
      // Print any parse errors;
      std::cerr << "Error parsing WebAssembly text file:" << std::endl;
      for(auto& error : parseErrors)
      {
         std::cerr << ":" << error.locus.describe() << ": " << error.message.c_str() << std::endl;
         std::cerr << error.locus.sourceLine << std::endl;
         std::cerr << std::setw(error.locus.column(8)) << "^" << std::endl;
      }
      FC_ASSERT( !"error parsing wast" );
   }

   try
   {
      // Serialize the WebAssembly module.
      Serialization::ArrayOutputStream stream;
      WASM::serialize(stream,module);
      return stream.getBytes();
   }
   catch(Serialization::FatalSerializationException exception)
   {
      std::cerr << "Error serializing WebAssembly binary file:" << std::endl;
      std::cerr << exception.message << std::endl;
      throw;
   }
}

read_only::get_info_results get_info() {
	auto& db = get_db();
	return {
		db.head_block_num(),
		db.last_irreversible_block_num(),
		db.head_block_id(),
		db.head_block_time(),
		db.head_block_producer(),
		std::bitset<64>(db.get_dynamic_global_properties().recent_slots_filled).to_string(),
		__builtin_popcountll(db.get_dynamic_global_properties().recent_slots_filled) / 64.0
	};
}

void sign_transaction(SignedTransaction& trx) {
   // TODO better error checking

//	const auto& public_keys = call(wallet_host, wallet_port, wallet_public_keys);
	const auto& public_keys = get_wm().get_public_keys();
	auto get_arg = fc::mutable_variant_object
		 ("transaction", trx)
		 ("available_keys", public_keys);

	auto ro_api = app().get_plugin<chain_plugin>().get_read_only_api();
	auto rw_api = app().get_plugin<chain_plugin>().get_read_write_api();

//	const auto& required_keys = call(host, port, get_required_keys, get_arg);
	eos::chain_apis::read_only::get_required_keys_params params = {fc::variant(trx),public_keys};
	eos::chain_apis::read_only::get_required_keys_result required_keys = ro_api.get_required_keys(params);
	// TODO determine chain id
//	fc::variants sign_args = {fc::variant(trx), required_keys["required_keys"], fc::variant(chain_id_type{})};

//	const auto& signed_trx = call(wallet_host, wallet_port, wallet_sign_trx, sign_args);
//	trx = signed_trx.as<SignedTransaction>();
	auto signed_trx = get_wm().sign_transaction(trx,required_keys.required_keys,chain_id_type{});
	trx = signed_trx;
}


string push_transaction( SignedTransaction& trx, bool sign ) {
    auto info = get_info();
    trx.expiration = info.head_block_time + 100; //chain.head_block_time() + 100;
    transaction_set_reference_block(trx, info.head_block_id);
    boost::sort( trx.scope );

    if (sign) {
       sign_transaction(trx);
    }

    auto v = fc::variant(trx);
	ilog(fc::json::to_string( trx ));

	auto rw = app().get_plugin<chain_plugin>().get_read_write_api();
	auto result = fc::json::to_string(rw.push_transaction(v.get_object()));
	ilog(result);
	return result;


    ProcessedTransaction pts = get_db().push_transaction(trx,sign);
    return get_db().transaction_to_variant(pts).get_string();
}

void create_key_(string& pub,string& priv){
	auto priv_ = fc::ecc::private_key::generate();
	auto pub_ = public_key_type( priv_.get_public_key() );
	pub = string(pub_);
	priv = string(key_to_wif(priv_.get_secret()));
}

string create_account__(Name creator, Name newaccount, public_key_type owner, public_key_type active, bool sign) {
	try {
		auto owner_auth   = eos::chain::Authority{1, {{owner, 1}}, {}};
		auto active_auth  = eos::chain::Authority{1, {{active, 1}}, {}};
		auto recovery_auth = eos::chain::Authority{1, {}, {{{creator, "active"}, 1}}};
		printf("%d\n",sign);

		uint64_t deposit = 1;
		SignedTransaction trx;
		trx.scope = sort_names({creator,config::EosContractName});
		transaction_emplace_message(trx, config::EosContractName, vector<types::AccountPermission>{{creator,"active"}}, "newaccount",
										   types::newaccount{creator, newaccount, owner_auth,
															 active_auth, recovery_auth, deposit});
		return fc::json::to_pretty_string(push_transaction(trx, sign));
	}catch(fc::assert_exception& e){
		elog(e.to_detail_string());
	}catch(fc::exception& e){
		elog(e.to_detail_string());
	}
	return "";
}

string create_account_(string creator, string newaccount, string owner, string active, int sign) {
	return create_account__(creator,newaccount,public_key_type(owner),public_key_type(active),sign);
}

class PyArray
{
public:
    PyArray(){
        arr = array_create();
    }
    void append(string s){
        array_append_string(arr,s);
    }
    void append(int n){
        array_append_int(arr,n);
    }
    void append(unsigned int n){
        array_append_int(arr,n);
    }
    void append(uint64_t n){
        array_append_uint64(arr,n);
    }
    void append(double n){
        array_append_double(arr,n);
    }
    PyObject *get(){
        return arr;
    }
private:
    PyObject *arr;
};


PyObject* get_info_(){
    PyArray arr;
    const chain_controller& db=get_db();

    arr.append(db.head_block_num());
    arr.append(db.last_irreversible_block_num());
    arr.append(db.head_block_id().str());
    arr.append(db.head_block_time().to_iso_string());
    arr.append(db.head_block_producer().toString());

    string recent_slots = std::bitset<64>(db.get_dynamic_global_properties().recent_slots_filled).to_string();
    arr.append(recent_slots);

    double participation_rate = __builtin_popcountll(db.get_dynamic_global_properties().recent_slots_filled) / 64.0;
    arr.append(participation_rate);
    return arr.get();
}
/*
{
  "previous": "0000000000000000000000000000000000000000000000000000000000000000",
  "timestamp": "2017-09-02T13:38:39",
  "transaction_merkle_root": "0000000000000000000000000000000000000000000000000000000000000000",
  "producer": "initr",
  "producer_changes": [],
  "producer_signature": "1f6a8a50b6993bfa07d0b195c4e5378d4f105e0c6cfe6beed5050948531b7000ef2d75f463d7a095d5b931c3923a78e04406308d2cbd2a5435422046363f948940",
  "cycles": [],
  "id": "00000001b107f27f37bb944c10952a758dc44129fe0310b7a1b76d47a67ad908",
  "block_num": 1,
  "refBlockPrefix": 1284815671
}
*/

PyObject *get_block_(char *num_or_id){
   PyArray arr;
   auto& db = get_db();

   string block_num_or_id(num_or_id);
   try {
      if (auto block = db.fetch_block_by_id(fc::json::from_string(block_num_or_id).as<chain::block_id_type>())){
         eos::chain_apis::read_only::get_block_results results = *block;

         arr.append(results.previous.str());
         arr.append(results.timestamp.to_iso_string());
         arr.append(results.transaction_merkle_root.str());
         arr.append(results.producer.toString());
         //producer_changes
         fc::variant v;
         fc::to_variant<unsigned char,65>(results.producer_signature,v);
         arr.append(fc::json::to_string(v));
         //cycles
         arr.append(results.id.str());
         arr.append(results.block_num);
         arr.append(results.refBlockPrefix);
      }
   } catch (fc::bad_cast_exception) {/* do nothing */}
   catch ( const fc::exception& e ) {
     elog((e.to_detail_string()));
   }

   try {
      if (auto block = db.fetch_block_by_number(fc::to_uint64(block_num_or_id))){
         eos::chain_apis::read_only::get_block_results results = *block;

         arr.append(results.previous.str());
         arr.append(results.timestamp.to_iso_string());
         arr.append(results.transaction_merkle_root.str());
         arr.append(results.producer.toString());
         //producer_changes
         fc::variant v;
         fc::to_variant<unsigned char,65>(results.producer_signature,v);
         arr.append(fc::json::to_string(v));
         //cycles
         arr.append(results.id.str());
         arr.append(results.block_num);
         arr.append(results.refBlockPrefix);
      }
   } catch (fc::bad_cast_exception) {/* do nothing */}
   catch ( const fc::exception& e ) {
     elog((e.to_detail_string()));
   }
   return arr.get();
}

/*
   struct get_account_results {
      Name                       name;
      uint64_t                   eos_balance       = 0;
      uint64_t                   staked_balance    = 0;
      uint64_t                   unstaking_balance = 0;
      fc::time_point_sec         last_unstaking_time;
      optional<producer_info>    producer;
      optional<types::Abi>       abi;
   };
*/
PyObject* get_account_(char *name){
   using namespace native::eos;
   PyArray arr;
   arr.append(name);

   auto& db = get_db();

   eos::chain_apis::read_only::get_account_results result;
   eos::chain_apis::read_only::get_account_params params = {name};

   result.name = params.name;

   const auto& d = db.get_database();
   const auto& accnt          = d.get<account_object,by_name>( params.name );
   const auto& balance        = d.get<BalanceObject,byOwnerName>( params.name );
   const auto& staked_balance = d.get<StakedBalanceObject,byOwnerName>( params.name );

   arr.append((uint64_t)balance.balance);
   arr.append((uint64_t)staked_balance.stakedBalance);
   arr.append((uint64_t)staked_balance.unstakingBalance);
   arr.append(staked_balance.lastUnstakingTime.to_iso_string());

   if( accnt.abi.size() > 4 ) {
      eos::types::Abi abi;
      fc::datastream<const char*> ds( accnt.abi.data(), accnt.abi.size() );
      fc::raw::unpack( ds, abi );     
      string s = fc::json::to_string(fc::variant(abi));
      arr.append(s);
   }
   return arr.get();
}

PyObject* get_accounts_(char *public_key){
	PyArray arr;
	if (public_key == NULL){
		return arr.get();
	}
	auto ro_api = app().get_plugin<account_history_plugin>().get_read_only_api();
	auto rw_api = app().get_plugin<account_history_plugin>().get_read_write_api();
	eos::account_history_apis::read_only::get_key_accounts_params params = {chain::public_key_type{}};
	eos::account_history_apis::read_only::get_key_accounts_results results= ro_api.get_key_accounts(params);

	for(auto it = results.account_names.begin();it!=results.account_names.end();++it){
		arr.append(string(*it));
	}
	return arr.get();
}

PyObject* get_controlled_accounts_(char *account_name){
	PyArray arr;
	if (account_name == NULL){
		return arr.get();
	}
	auto ro_api = app().get_plugin<account_history_plugin>().get_read_only_api();
	eos::account_history_apis::read_only::get_controlled_accounts_params params = {Name(account_name)};
	eos::account_history_apis::read_only::get_controlled_accounts_results results = ro_api.get_controlled_accounts(params);
	for(auto it = results.controlled_accounts.begin();it!=results.controlled_accounts.end();it++){
		arr.append(string(*it));
	}
	return arr.get();
}

string get_transaction_(string id){
	eos::account_history_apis::read_only::get_transaction_params params = {chain::transaction_id_type(id)};

	auto ro_api = app().get_plugin<account_history_plugin>().get_read_only_api();
	eos::account_history_apis::read_only::get_transaction_results results = ro_api.get_transaction(params);
	return fc::json::to_string(results);
}

string get_transactions_(string account_name,int skip_seq,int num_seq){
	const eos::account_history_apis::read_only::get_transactions_params params = {
			chain::AccountName(account_name),
			skip_seq,
			num_seq
	};
	auto ro_api = app().get_plugin<account_history_plugin>().get_read_only_api();
	eos::account_history_apis::read_only::get_transactions_results results = ro_api.get_transactions(params);
	return fc::json::to_string(results);
}









