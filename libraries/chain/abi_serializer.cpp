/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/chain/abi_serializer.hpp>
#include <eosio/chain/contract_types.hpp>
#include <eosio/chain/authority.hpp>
#include <eosio/chain/chain_config.hpp>
#include <eosio/chain/transaction.hpp>
#include <eosio/chain/asset.hpp>
#include <eosio/chain/exceptions.hpp>
#include <fc/io/raw.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <fc/io/varint.hpp>

using namespace boost;

namespace eosio { namespace chain {

   const size_t abi_serializer::max_recursion_depth;

   using boost::algorithm::ends_with;
   using std::string;

   template <typename T>
   inline fc::variant variant_from_stream(fc::datastream<const char*>& stream) {
      T temp;
      fc::raw::unpack( stream, temp );
      return fc::variant(temp);
   }

   template <typename T>
   auto pack_unpack() {
      return std::make_pair<abi_serializer::unpack_function, abi_serializer::pack_function>(
         []( fc::datastream<const char*>& stream, bool is_array, bool is_optional) -> fc::variant  {
            if( is_array )
               return variant_from_stream<vector<T>>(stream);
            else if ( is_optional )
               return variant_from_stream<optional<T>>(stream);
            return variant_from_stream<T>(stream);
         },
         []( const fc::variant& var, fc::datastream<char*>& ds, bool is_array, bool is_optional ){
            if( is_array )
               fc::raw::pack( ds, var.as<vector<T>>() );
            else if ( is_optional )
               fc::raw::pack( ds, var.as<optional<T>>() );
            else
               fc::raw::pack( ds,  var.as<T>());
         }
      );
   }

   abi_serializer::abi_serializer( const abi_def& abi, const fc::microseconds& max_serialization_time ) {
      configure_built_in_types();
      set_abi(abi, max_serialization_time);
   }

   void abi_serializer::add_specialized_unpack_pack( const string& name,
                                                     std::pair<abi_serializer::unpack_function, abi_serializer::pack_function> unpack_pack ) {
      built_in_types[name] = std::move( unpack_pack );
   }

   void abi_serializer::configure_built_in_types() {

      built_in_types.emplace("bool",                      pack_unpack<uint8_t>());
      built_in_types.emplace("int8",                      pack_unpack<int8_t>());
      built_in_types.emplace("uint8",                     pack_unpack<uint8_t>());
      built_in_types.emplace("int16",                     pack_unpack<int16_t>());
      built_in_types.emplace("uint16",                    pack_unpack<uint16_t>());
      built_in_types.emplace("int32",                     pack_unpack<int32_t>());
      built_in_types.emplace("uint32",                    pack_unpack<uint32_t>());
      built_in_types.emplace("int64",                     pack_unpack<int64_t>());
      built_in_types.emplace("uint64",                    pack_unpack<uint64_t>());
      built_in_types.emplace("int128",                    pack_unpack<int128_t>());
      built_in_types.emplace("uint128",                   pack_unpack<uint128_t>());
      built_in_types.emplace("varint32",                  pack_unpack<fc::signed_int>());
      built_in_types.emplace("varuint32",                 pack_unpack<fc::unsigned_int>());

      // TODO: Add proper support for floating point types. For now this is good enough.
      built_in_types.emplace("float32",                   pack_unpack<float>());
      built_in_types.emplace("float64",                   pack_unpack<double>());
      built_in_types.emplace("float128",                  pack_unpack<uint128_t>());

      built_in_types.emplace("time_point",                pack_unpack<fc::time_point>());
      built_in_types.emplace("time_point_sec",            pack_unpack<fc::time_point_sec>());
      built_in_types.emplace("block_timestamp_type",      pack_unpack<block_timestamp_type>());

      built_in_types.emplace("name",                      pack_unpack<name>());

      built_in_types.emplace("bytes",                     pack_unpack<bytes>());
      built_in_types.emplace("string",                    pack_unpack<string>());

      built_in_types.emplace("checksum160",               pack_unpack<checksum160_type>());
      built_in_types.emplace("checksum256",               pack_unpack<checksum256_type>());
      built_in_types.emplace("checksum512",               pack_unpack<checksum512_type>());

      built_in_types.emplace("public_key",                pack_unpack<public_key_type>());
      built_in_types.emplace("signature",                 pack_unpack<signature_type>());

      built_in_types.emplace("symbol",                    pack_unpack<symbol>());
      built_in_types.emplace("symbol_code",               pack_unpack<symbol_code>());
      built_in_types.emplace("asset",                     pack_unpack<asset>());
      built_in_types.emplace("extended_asset",            pack_unpack<extended_asset>());
   }

   void abi_serializer::set_abi(const abi_def& abi, const fc::microseconds& max_serialization_time) {
      impl::abi_traverse_context ctx(max_serialization_time);

      EOS_ASSERT(starts_with(abi.version, "eosio::abi/1."), unsupported_abi_version_exception, "ABI has an unsupported version");

      typedefs.clear();
      structs.clear();
      actions.clear();
      tables.clear();
      error_messages.clear();
      variants.clear();

      for( const auto& st : abi.structs )
         structs[st.name] = st;

      for( const auto& td : abi.types ) {
         EOS_ASSERT(_is_type(td.type, ctx), invalid_type_inside_abi, "invalid type", ("type",td.type));
         EOS_ASSERT(!_is_type(td.new_type_name, ctx), duplicate_abi_type_def_exception, "type already exists", ("new_type_name",td.new_type_name));
         typedefs[td.new_type_name] = td.type;
      }

      for( const auto& a : abi.actions )
         actions[a.name] = a.type;

      for( const auto& t : abi.tables )
         tables[t.name] = t.type;

      for( const auto& e : abi.error_messages )
         error_messages[e.error_code] = e.error_msg;

      for( const auto& v : abi.variants.value )
         variants[v.name] = v;

      /**
       *  The ABI vector may contain duplicates which would make it
       *  an invalid ABI
       */
      EOS_ASSERT( typedefs.size() == abi.types.size(), duplicate_abi_type_def_exception, "duplicate type definition detected" );
      EOS_ASSERT( structs.size() == abi.structs.size(), duplicate_abi_struct_def_exception, "duplicate struct definition detected" );
      EOS_ASSERT( actions.size() == abi.actions.size(), duplicate_abi_action_def_exception, "duplicate action definition detected" );
      EOS_ASSERT( tables.size() == abi.tables.size(), duplicate_abi_table_def_exception, "duplicate table definition detected" );
      EOS_ASSERT( error_messages.size() == abi.error_messages.size(), duplicate_abi_err_msg_def_exception, "duplicate error message definition detected" );
      EOS_ASSERT( variants.size() == abi.variants.value.size(), duplicate_abi_variant_def_exception, "duplicate variant definition detected" );

      validate(ctx);
   }

   bool abi_serializer::is_builtin_type(const type_name& type)const {
      return built_in_types.find(type) != built_in_types.end();
   }

   bool abi_serializer::is_integer(const type_name& type) const {
      string stype = type;
      return boost::starts_with(stype, "uint") || boost::starts_with(stype, "int");
   }

   int abi_serializer::get_integer_size(const type_name& type) const {
      string stype = type;
      EOS_ASSERT( is_integer(type), invalid_type_inside_abi, "${stype} is not an integer type", ("stype",stype));
      if( boost::starts_with(stype, "uint") ) {
         return boost::lexical_cast<int>(stype.substr(4));
      } else {
         return boost::lexical_cast<int>(stype.substr(3));
      }
   }

   bool abi_serializer::is_struct(const type_name& type)const {
      return structs.find(resolve_type(type)) != structs.end();
   }

   bool abi_serializer::is_array(const type_name& type)const {
      return ends_with(string(type), "[]");
   }

   bool abi_serializer::is_optional(const type_name& type)const {
      return ends_with(string(type), "?");
   }

   bool abi_serializer::is_type(const type_name& type, const fc::microseconds& max_serialization_time)const {
      impl::abi_traverse_context ctx(max_serialization_time);
      return _is_type(type, ctx);
   }

   type_name abi_serializer::fundamental_type(const type_name& type)const {
      if( is_array(type) ) {
         return type_name(string(type).substr(0, type.size()-2));
      } else if ( is_optional(type) ) {
         return type_name(string(type).substr(0, type.size()-1));
      } else {
       return type;
      }
   }

   type_name abi_serializer::_remove_bin_extension(const type_name& type) {
      if( ends_with(type, "$") )
         return type.substr(0, type.size()-1);
      else
         return type;
   }

   bool abi_serializer::_is_type(const type_name& rtype, impl::abi_traverse_context& ctx )const {
      auto h = ctx.enter_scope();
      auto type = fundamental_type(rtype);
      if( built_in_types.find(type) != built_in_types.end() ) return true;
      if( typedefs.find(type) != typedefs.end() ) return _is_type(typedefs.find(type)->second, ctx);
      if( structs.find(type) != structs.end() ) return true;
      if( variants.find(type) != variants.end() ) return true;
      return false;
   }

   const struct_def& abi_serializer::get_struct(const type_name& type)const {
      auto itr = structs.find(resolve_type(type) );
      EOS_ASSERT( itr != structs.end(), invalid_type_inside_abi, "Unknown struct ${type}", ("type",type) );
      return itr->second;
   }

   void abi_serializer::validate( impl::abi_traverse_context& ctx )const {
      for( const auto& t : typedefs ) { try {
         vector<type_name> types_seen{t.first, t.second};
         auto itr = typedefs.find(t.second);
         while( itr != typedefs.end() ) {
            ctx.check_deadline();
            EOS_ASSERT( find(types_seen.begin(), types_seen.end(), itr->second) == types_seen.end(), abi_circular_def_exception, "Circular reference in type ${type}", ("type",t.first) );
            types_seen.emplace_back(itr->second);
            itr = typedefs.find(itr->second);
         }
      } FC_CAPTURE_AND_RETHROW( (t) ) }
      for( const auto& t : typedefs ) { try {
         EOS_ASSERT(_is_type(t.second, ctx), invalid_type_inside_abi, "", ("type",t.second) );
      } FC_CAPTURE_AND_RETHROW( (t) ) }
      for( const auto& s : structs ) { try {
         if( s.second.base != type_name() ) {
            struct_def current = s.second;
            vector<type_name> types_seen{current.name};
            while( current.base != type_name() ) {
               ctx.check_deadline();
               const auto& base = get_struct(current.base); //<-- force struct to inherit from another struct
               EOS_ASSERT( find(types_seen.begin(), types_seen.end(), base.name) == types_seen.end(), abi_circular_def_exception, "Circular reference in struct ${type}", ("type",s.second.name) );
               types_seen.emplace_back(base.name);
               current = base;
            }
         }
         for( const auto& field : s.second.fields ) { try {
            ctx.check_deadline();
            EOS_ASSERT(_is_type(_remove_bin_extension(field.type), ctx), invalid_type_inside_abi, "", ("type",field.type) );
         } FC_CAPTURE_AND_RETHROW( (field) ) }
      } FC_CAPTURE_AND_RETHROW( (s) ) }
      for( const auto& s : variants ) { try {
         for( const auto& type : s.second.types ) { try {
            ctx.check_deadline();
            EOS_ASSERT(_is_type(type, ctx), invalid_type_inside_abi, "", ("type",type) );
         } FC_CAPTURE_AND_RETHROW( (type) ) }
      } FC_CAPTURE_AND_RETHROW( (s) ) }
      for( const auto& a : actions ) { try {
        ctx.check_deadline();
        EOS_ASSERT(_is_type(a.second, ctx), invalid_type_inside_abi, "", ("type",a.second) );
      } FC_CAPTURE_AND_RETHROW( (a)  ) }

      for( const auto& t : tables ) { try {
        ctx.check_deadline();
        EOS_ASSERT(_is_type(t.second, ctx), invalid_type_inside_abi, "", ("type",t.second) );
      } FC_CAPTURE_AND_RETHROW( (t)  ) }
   }

   type_name abi_serializer::resolve_type(const type_name& type)const {
      auto itr = typedefs.find(type);
      if( itr != typedefs.end() ) {
         for( auto i = typedefs.size(); i > 0; --i ) { // avoid infinite recursion
            const type_name& t = itr->second;
            itr = typedefs.find( t );
            if( itr == typedefs.end() ) return t;
         }
      }
      return type;
   }

   void abi_serializer::_binary_to_variant( const type_name& type, fc::datastream<const char *>& stream,
                                            fc::mutable_variant_object& obj, impl::binary_to_variant_context& ctx )const
   {
      auto h = ctx.enter_scope();
      const auto& st = get_struct(type);
      if( st.base != type_name() ) {
         _binary_to_variant(resolve_type(st.base), stream, obj, ctx);
      }
      for( const auto& field : st.fields ) {
         if( !stream.remaining() && ends_with(field.type, "$") )
            continue;
         obj( field.name, _binary_to_variant(resolve_type(_remove_bin_extension(field.type)), stream, ctx) );
      }
   }

   fc::variant abi_serializer::_binary_to_variant( const type_name& type, fc::datastream<const char *>& stream,
                                                   impl::binary_to_variant_context& ctx )const
   {
      auto h = ctx.enter_scope();
      type_name rtype = resolve_type(type);
      auto ftype = fundamental_type(rtype);
      auto btype = built_in_types.find(ftype );
      if( btype != built_in_types.end() ) {
         return btype->second.first(stream, is_array(rtype), is_optional(rtype));
      }
      if ( is_array(rtype) ) {
        fc::unsigned_int size;
        fc::raw::unpack(stream, size);
        vector<fc::variant> vars;
        for( decltype(size.value) i = 0; i < size; ++i ) {
           auto v = _binary_to_variant(ftype, stream, ctx);
           EOS_ASSERT( !v.is_null(), unpack_exception, "Invalid packed array" );
           vars.emplace_back(std::move(v));
        }
        EOS_ASSERT( vars.size() == size.value,
                    unpack_exception,
                    "packed size does not match unpacked array size, packed size ${p} actual size ${a}",
                    ("p", size)("a", vars.size()) );
        return fc::variant( std::move(vars) );
      } else if ( is_optional(rtype) ) {
        char flag;
        fc::raw::unpack(stream, flag);
        return flag ? _binary_to_variant(ftype, stream, ctx) : fc::variant();
      } else {
         auto v = variants.find(rtype);
         if( v != variants.end() ) {
            fc::unsigned_int select;
            fc::raw::unpack(stream, select);
            EOS_ASSERT( (size_t)select < v->second.types.size(), unpack_exception, "Invalid packed variant" );
            return vector<fc::variant>{v->second.types[select], _binary_to_variant(v->second.types[select], stream, ctx)};
         }
      }

      fc::mutable_variant_object mvo;
      _binary_to_variant(rtype, stream, mvo, ctx);
      EOS_ASSERT( mvo.size() > 0, unpack_exception, "Unable to unpack stream ${type}", ("type", type) );
      return fc::variant( std::move(mvo) );
   }

   fc::variant abi_serializer::_binary_to_variant( const type_name& type, const bytes& binary, impl::binary_to_variant_context& ctx )const
   {
      auto h = ctx.enter_scope();
      fc::datastream<const char*> ds( binary.data(), binary.size() );
      return _binary_to_variant(type, ds, ctx);
   }

   fc::variant abi_serializer::binary_to_variant(const type_name& type, const bytes& binary, const fc::microseconds& max_serialization_time)const {
      impl::binary_to_variant_context ctx(max_serialization_time);
      return _binary_to_variant(type, binary, ctx);
   }

   fc::variant abi_serializer::binary_to_variant(const type_name& type, fc::datastream<const char*>& binary, const fc::microseconds& max_serialization_time)const {
      impl::binary_to_variant_context ctx(max_serialization_time);
      return _binary_to_variant(type, binary, ctx);
   }

   void abi_serializer::_variant_to_binary( const type_name& type, const fc::variant& var, fc::datastream<char *>& ds, impl::variant_to_binary_context& ctx )const
   { try {
      auto h = ctx.enter_scope();
      auto rtype = resolve_type(type);

      auto btype = built_in_types.find(fundamental_type(rtype));
      if( btype != built_in_types.end() ) {
         btype->second.second(var, ds, is_array(rtype), is_optional(rtype));
      } else if ( is_array(rtype) ) {
         vector<fc::variant> vars = var.get_array();
         fc::raw::pack(ds, (fc::unsigned_int)vars.size());
         auto h1 = ctx.push_to_path( fundamental_type(rtype), ctx.is_path_empty() );
         int64_t i = 0;
         for (const auto& var : vars) {
            ctx.set_array_index_of_path_back(i);
            auto h2 = ctx.disallow_extensions_unless(false);
           _variant_to_binary(fundamental_type(rtype), var, ds, ctx);
           ++i;
         }
      } else if ( variants.find(rtype) != variants.end() ) {
         auto& v = variants.find(rtype)->second;
         auto h1 = ctx.push_to_path( v.name, ctx.is_path_empty() );
         EOS_ASSERT( var.is_array() && var.size() == 2, pack_exception,
                    "Expected input to be an array of two items while processing variant '${p}'", ("p", ctx.get_path_string()) );
         EOS_ASSERT( var[size_t(0)].is_string(), pack_exception,
                    "Encountered non-string as first item of input array while processing variant '${p}'", ("p", ctx.get_path_string()) );
         auto variant_type_str = var[size_t(0)].get_string();
         auto it = find(v.types.begin(), v.types.end(), variant_type_str);
         EOS_ASSERT( it != v.types.end(), pack_exception,
                     "Specified type '${t}' in input array is not valid within the variant '${p}'",
                     ("t", variant_type_str)("p", ctx.get_path_string()) );
         fc::raw::pack(ds, fc::unsigned_int(it - v.types.begin()));
         std::stringstream s;
         s << "<variant(" << (it - v.types.begin()) << ")=" << *it << ">";
         auto h3 = ctx.push_to_path(s.str());
         _variant_to_binary( *it, var[size_t(1)], ds, ctx );
      } else {
         const auto& st = get_struct(rtype);

         auto h1 = ctx.push_to_path( st.name, ctx.is_path_empty() );

         if( var.is_object() ) {
            const auto& vo = var.get_object();

            if( st.base != type_name() ) {
               auto h2 = ctx.disallow_extensions_unless(false);
               _variant_to_binary(resolve_type(st.base), var, ds, ctx);
            }
            bool extension_encountered = false;
            for( const auto& field : st.fields ) {
               if( vo.contains( string(field.name).c_str() ) ) {
                  if( extension_encountered )
                     EOS_THROW( pack_exception, "Unexpected field '${f}' found in input object while processing struct '${p}'", ("f",field.name)("p",ctx.get_path_string()) );
                  {
                     auto h2 = ctx.disallow_extensions_unless( &field == &st.fields.back() );
                     auto h3 = ctx.push_to_path( field.name );
                     _variant_to_binary(_remove_bin_extension(field.type), vo[field.name], ds, ctx);
                  }
               } else if( ends_with(field.type, "$") && ctx.extensions_allowed() ) {
                  extension_encountered = true;
               } else if( extension_encountered ) {
                  EOS_THROW( pack_exception, "Encountered field '${f}' without binary extension designation while processing struct '${p}'", ("f",field.name)("p",ctx.get_path_string()) );
               } else {
                  EOS_THROW( pack_exception, "Missing field '${f}' in input object while processing struct '${p}'", ("f",field.name)("p",ctx.get_path_string()) );
               }
            }
         } else if( var.is_array() ) {
            const auto& va = var.get_array();
            EOS_ASSERT( st.base == type_name(), invalid_type_inside_abi,
                        "Using input array to specify the fields of the derived struct '${p}'; input arrays are currently only allowed for structs without a base",
                        ("p",ctx.get_path_string()) );
            uint32_t i = 0;
            for( const auto& field : st.fields ) {
               if( va.size() > i ) {
                  auto h2 = ctx.disallow_extensions_unless( &field == &st.fields.back() );
                  auto h3 = ctx.push_to_path( field.name );
                  _variant_to_binary(_remove_bin_extension(field.type), va[i], ds, ctx);
               } else if( ends_with(field.type, "$") && ctx.extensions_allowed() ) {
                  break;
               } else {
                  EOS_THROW( pack_exception, "Early end to input array specifying the fields of struct '${p}'; require input for field '${f}'",
                             ("p", ctx.get_path_string())("f", field.name) );
               }
               ++i;
            }
         } else {
            EOS_THROW( pack_exception, "Unexpected input encountered while processing struct '${p}'", ("p",ctx.get_path_string()) );
         }
      }
   } FC_CAPTURE_AND_RETHROW( (type)(var) ) }

   bytes abi_serializer::_variant_to_binary( const type_name& type, const fc::variant& var, impl::variant_to_binary_context& ctx )const
   { try {
      auto h = ctx.enter_scope();
      if( !_is_type(type, ctx) ) {
         return var.as<bytes>();
      }

      bytes temp( 1024*1024 );
      fc::datastream<char*> ds(temp.data(), temp.size() );
      _variant_to_binary(type, var, ds, ctx);
      temp.resize(ds.tellp());
      return temp;
   } FC_CAPTURE_AND_RETHROW( (type)(var) ) }

   bytes abi_serializer::variant_to_binary(const type_name& type, const fc::variant& var, const fc::microseconds& max_serialization_time)const {
      impl::variant_to_binary_context ctx(max_serialization_time);
      return _variant_to_binary(type, var, ctx);
   }

   void  abi_serializer::variant_to_binary(const type_name& type, const fc::variant& var, fc::datastream<char*>& ds, const fc::microseconds& max_serialization_time)const {
      impl::variant_to_binary_context ctx(max_serialization_time);
      _variant_to_binary(type, var, ds, ctx);
   }

   type_name abi_serializer::get_action_type(name action)const {
      auto itr = actions.find(action);
      if( itr != actions.end() ) return itr->second;
      return type_name();
   }
   type_name abi_serializer::get_table_type(name action)const {
      auto itr = tables.find(action);
      if( itr != tables.end() ) return itr->second;
      return type_name();
   }

   optional<string> abi_serializer::get_error_message( uint64_t error_code )const {
      auto itr = error_messages.find( error_code );
      if( itr == error_messages.end() )
         return optional<string>();

      return itr->second;
   }

} }
