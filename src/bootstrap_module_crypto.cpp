#include "lua/bind.h"
#include "llae/async_bind.h"

#include "crypto/bignum.h"
#include "crypto/cert.h"
#include "crypto/cipher.h"
#include "crypto/crypto.h"
#include "crypto/ecp.h"
#include "crypto/hmac.h"
#include "crypto/md.h"
#include "crypto/pk.h"
#include "crypto/random.h"
#include "crypto/rsa.h"
#include "crypto/luabind.h"


/* class crypto::bignum */
static void luabind_crypto_bignum(lua::state& l) {
    
    
    lua::bind::function(l,"new",&crypto::lua_bignum_new);
    lua::bind::function(l,"random",&crypto::bignum::create_random);
    lua::bind::function(l,"is0",&crypto::bignum::is0);
    lua::bind::function(l,"get_bit",&crypto::bignum::get_bit);
    lua::bind::function(l,"set_bit",&crypto::bignum::set_bit);
    lua::bind::function(l,"self_add",&crypto::lua_bignum_self_add);
    lua::bind::function(l,"self_mul",&crypto::lua_bignum_self_mul);
    lua::bind::function(l,"self_sub",&crypto::lua_bignum_self_sub);
    lua::bind::function(l,"self_lshift",&crypto::bignum::self_lshift);
    lua::bind::function(l,"self_rshift",&crypto::bignum::self_rshift);
    lua::bind::function(l,"mul",&crypto::lua_bignum_mul);
    lua::bind::function(l,"__mul",&crypto::lua_bignum_mul);
    lua::bind::function(l,"div",&crypto::lua_bignum_div);
    lua::bind::function(l,"__div",&crypto::lua_bignum_div);
    lua::bind::function(l,"add",&crypto::lua_bignum_add);
    lua::bind::function(l,"__add",&crypto::lua_bignum_add);
    lua::bind::function(l,"sub",&crypto::lua_bignum_sub);
    lua::bind::function(l,"__sub",&crypto::lua_bignum_sub);
    lua::bind::function(l,"mod",&crypto::lua_bignum_mod);
    lua::bind::function(l,"exp_mod",&crypto::bignum::exp_mod);
    lua::bind::function(l,"tostring",&crypto::bignum::to_string);
    lua::bind::function(l,"__tostring",&crypto::bignum::to_string);
    lua::bind::function(l,"write",&crypto::bignum::write);
    lua::bind::function(l,"write_le",&crypto::bignum::write_le);
    lua::bind::function(l,"read",&crypto::bignum::read);
    lua::bind::function(l,"read_le",&crypto::bignum::read_le);
    lua::bind::function(l,"__lt",&crypto::lua_bignum_less);
    lua::bind::function(l,"__le",&crypto::lua_bignum_lequal);
    lua::bind::function(l,"equal",&crypto::lua_bignum_equal);
    lua::bind::function(l,"__eq",&crypto::lua_bignum_equal);
    
    
    
}

/* class crypto::cert_time */
static void luabind_crypto_cert_time(lua::state& l) {
    
    
    
    lua::bind::field(l,"year",&crypto::cert_time::year);
    lua::bind::field(l,"mon",&crypto::cert_time::mon);
    lua::bind::field(l,"day",&crypto::cert_time::day);
    lua::bind::field(l,"hour",&crypto::cert_time::hour);
    lua::bind::field(l,"min",&crypto::cert_time::min);
    lua::bind::field(l,"sec",&crypto::cert_time::sec);
    
    
}

/* class crypto::cert */
static void luabind_crypto_cert(lua::state& l) {
    
    
    lua::bind::constructor<crypto::cert>(l);
    
    
    
    lua::bind::function(l,"parse",&crypto::cert::parse);
    lua::bind::function(l,"is_loaded",&crypto::cert::is_loaded);
    lua::bind::function(l,"get_version",&crypto::cert::get_version);
    lua::bind::function(l,"get_subject",&crypto::cert::get_subject);
    lua::bind::function(l,"get_issuer",&crypto::cert::get_issuer);
    lua::bind::function(l,"get_serial",&crypto::cert::get_serial);
    lua::bind::function(l,"get_not_before",&crypto::cert::get_not_before);
    lua::bind::function(l,"get_not_after",&crypto::cert::get_not_after);
    lua::bind::function(l,"get_signature_algorithm",&crypto::cert::get_signature_algorithm);
    lua::bind::function(l,"get_raw",&crypto::cert::get_raw);
    lua::bind::function(l,"get_subject_der",&crypto::cert::get_subject_der);
    lua::bind::function(l,"get_issuer_der",&crypto::cert::get_issuer_der);
    lua::bind::function(l,"get_public_key_der",&crypto::cert::get_public_key_der);
    lua::bind::function(l,"get_public_key",&crypto::cert::get_public_key);
    lua::bind::function(l,"get_subject_hash_old",&crypto::cert::get_subject_hash_old);
    lua::bind::function(l,"get_issuer_hash_old",&crypto::cert::get_issuer_hash_old);
    lua::bind::function(l,"is_expired",&crypto::cert::is_expired);
    lua::bind::function(l,"is_not_yet_valid",&crypto::cert::is_not_yet_valid);
    lua::bind::function(l,"is_valid_now",&crypto::cert::is_valid_now);
    
    
    
}

/* class crypto::cipher */
static void luabind_crypto_cipher(lua::state& l) {
    
    
    lua::bind::function(l,"set_iv",&crypto::cipher::set_iv);
    lua::bind::function(l,"set_key",&crypto::cipher::set_key);
    lua::bind::function(l,"set_padding",&crypto::cipher::set_padding);
    lua::bind::function(l,"reset",&crypto::cipher::reset);
    llae::async_function(l,"update",&crypto::lua_cipher_update);
    llae::async_function(l,"update_ad",&crypto::cipher::async_update_ad);
    lua::bind::function(l,"write_tag",&crypto::cipher::write_tag);
    lua::bind::function(l,"check_tag",&crypto::cipher::check_tag);
    llae::async_function(l,"finish",&crypto::cipher::async_finish);
    llae::async_function(l,"crypt",&crypto::cipher::async_crypt);
    llae::async_function(l,"auth_encrypt",&crypto::cipher::async_auth_encrypt);
    llae::async_function(l,"auth_decrypt",&crypto::cipher::async_auth_decrypt);
    lua::bind::function(l,"get_block_size",&crypto::cipher::get_block_size);
    lua::bind::function(l,"get_iv_size",&crypto::cipher::get_iv_size);
    lua::bind::function(l,"get_tag_size",&crypto::cipher::get_tag_size);
    lua::bind::function(l,"new",&crypto::cipher::create);
    
    
    
}

/* class crypto::ecp_point */
static void luabind_crypto_ecp_point(lua::state& l) {
    
    
    lua::bind::constructor<crypto::ecp_point>(l);
    
    
    
    lua::bind::function(l,"set_zero",&crypto::ecp_point::set_zero);
    lua::bind::function(l,"is_zero",&crypto::ecp_point::is_zero);
    lua::bind::function(l,"cmp",&crypto::ecp_point::cmp);
    lua::bind::function(l,"read_string",&crypto::ecp_point::read_string);
    
    
    
}

/* class crypto::ecp */
static void luabind_crypto_ecp(lua::state& l) {
    
    
    lua::bind::function(l,"new",&crypto::ecp::create);
    lua::bind::function(l,"point_read_binary",&crypto::ecp::point_read_binary);
    lua::bind::function(l,"point_write_binary",&crypto::ecp::point_write_binary);
    lua::bind::function(l,"check_pubkey",&crypto::ecp::check_pubkey);
    lua::bind::function(l,"check_privkey",&crypto::ecp::check_privkey);
    lua::bind::function(l,"ecdsa_verify",&crypto::ecp::ecdsa_verify);
    lua::bind::function(l,"ecdsa_sign",&crypto::ecp::ecdsa_sign);
    lua::bind::function(l,"gen_keypair",&crypto::ecp::gen_keypair);
    lua::bind::function(l,"gen_privkey",&crypto::ecp::gen_privkey);
    lua::bind::function(l,"gen_pubkey",&crypto::ecp::gen_pubkey);
    lua::bind::function(l,"set_random_data",&crypto::ecp::set_random_data);
    lua::bind::function(l,"set_random",&crypto::ecp::set_random);
    lua::bind::function(l,"ecdh_gen_public",&crypto::ecp::ecdh_gen_public);
    lua::bind::function(l,"ecdh_compute_shared",&crypto::ecp::ecdh_compute_shared);
    lua::bind::function(l,"scalar_mul",&crypto::ecp::scalar_mul);
    
    
    
}

/* class crypto::hmac */
static void luabind_crypto_hmac(lua::state& l) {
    
    
    lua::bind::function(l,"new",&crypto::lua_hmac_create);
    lua::bind::function(l,"reset",&crypto::hmac::reset);
    llae::async_function(l,"start",&crypto::hmac::async_start);
    llae::async_function(l,"update",&crypto::lua_hmac_update);
    llae::async_function(l,"finish",&crypto::hmac::async_finish);
    
    
    
}

/* class crypto::md */
static void luabind_crypto_md(lua::state& l) {
    
    
    lua::bind::function(l,"new",&crypto::lua_md_create);
    llae::async_function(l,"update",&crypto::lua_md_update);
    llae::async_function(l,"finish",&crypto::md::async_finish);
    lua::bind::function(l,"get_length",&crypto::lua_md_get_length);
    
    
    
}

/* class crypto::pk */
static void luabind_crypto_pk(lua::state& l) {
    
    
    lua::bind::constructor<crypto::pk>(l);
    
    
    
    lua::bind::function(l,"parse_public_key",&crypto::pk::parse_public_key);
    lua::bind::function(l,"parse_private_key",&crypto::pk::parse_private_key);
    lua::bind::function(l,"get_name",&crypto::pk::get_name);
    lua::bind::function(l,"get_rsa",&crypto::pk::get_rsa);
    llae::async_function(l,"encrypt",&crypto::pk::async_encrypt);
    llae::async_function(l,"decrypt",&crypto::pk::async_decrypt);
    
    
    
}

/* class crypto::entropy */
static void luabind_crypto_entropy(lua::state& l) {
    
    
    lua::bind::constructor<crypto::entropy>(l);
    
    
    
    lua::bind::function(l,"update_manual",&crypto::entropy::update_manual);
    
    
    
}

/* class crypto::random */
static void luabind_crypto_random(lua::state& l) {
    
    
    lua::bind::constructor<crypto::random, crypto::entropy_ptr&&>(l);
    
    
    
    lua::bind::function(l,"update",&crypto::random::update);
    lua::bind::function(l,"seed",&crypto::random::seed);
    
    
    
}

/* class crypto::rsa_base */
static void luabind_crypto_rsa_base(lua::state& l) {
    
    
    lua::bind::function(l,"get_len",&crypto::rsa_base::get_len);
    lua::bind::function(l,"set_padding",&crypto::rsa_base::set_padding);
    lua::bind::function(l,"sync_public",&crypto::rsa_base::sync_public);
    llae::async_function(l,"public",&crypto::rsa_base::async_public);
    
    
    
}


int luaopen_crypto(lua_State* L) {
    lua::state l(L);
    
    lua::bind::object<crypto::bignum>::register_metatable(l, &luabind_crypto_bignum);
    
    lua::bind::object<crypto::cert_time>::register_metatable(l, &luabind_crypto_cert_time);
    
    lua::bind::object<crypto::cert>::register_metatable(l, &luabind_crypto_cert);
    
    lua::bind::object<crypto::cipher>::register_metatable(l, &luabind_crypto_cipher);
    
    lua::bind::object<crypto::ecp_point>::register_metatable(l, &luabind_crypto_ecp_point);
    
    lua::bind::object<crypto::ecp>::register_metatable(l, &luabind_crypto_ecp);
    
    lua::bind::object<crypto::hmac>::register_metatable(l, &luabind_crypto_hmac);
    
    lua::bind::object<crypto::md>::register_metatable(l, &luabind_crypto_md);
    
    lua::bind::object<crypto::pk>::register_metatable(l, &luabind_crypto_pk);
    
    lua::bind::object<crypto::entropy>::register_metatable(l, &luabind_crypto_entropy);
    
    lua::bind::object<crypto::random>::register_metatable(l, &luabind_crypto_random);
    
    lua::bind::object<crypto::rsa_base>::register_metatable(l, &luabind_crypto_rsa_base);
    
    l.createtable();
    
    lua::bind::object<crypto::bignum>::get_metatable(l);
    l.setfield(-2,"bignum");
    
    lua::bind::object<crypto::cert_time>::get_metatable(l);
    l.setfield(-2,"cert_time");
    
    lua::bind::object<crypto::cert>::get_metatable(l);
    l.setfield(-2,"cert");
    
    lua::bind::object<crypto::cipher>::get_metatable(l);
    l.setfield(-2,"cipher");
    
    lua::bind::object<crypto::ecp_point>::get_metatable(l);
    l.setfield(-2,"ecp_point");
    
    lua::bind::object<crypto::ecp>::get_metatable(l);
    l.setfield(-2,"ecp");
    
    lua::bind::object<crypto::hmac>::get_metatable(l);
    l.setfield(-2,"hmac");
    
    lua::bind::object<crypto::md>::get_metatable(l);
    l.setfield(-2,"md");
    
    lua::bind::object<crypto::pk>::get_metatable(l);
    l.setfield(-2,"pk");
    
    lua::bind::object<crypto::entropy>::get_metatable(l);
    l.setfield(-2,"entropy");
    
    lua::bind::object<crypto::random>::get_metatable(l);
    l.setfield(-2,"random");
    
    lua::bind::object<crypto::rsa_base>::get_metatable(l);
    l.setfield(-2,"rsa_base");
    
    
    llae::async_function(l,"crc32",&crypto::async_crc32);
    llae::async_function(l,"hkdf",&crypto::lua_hkdf);
    
    
    lua::bind::value(l, "ECP_PF_COMPRESSED", crypto::ECP_PF_COMPRESSED);
    lua::bind::value(l, "ECP_PF_UNCOMPRESSED", crypto::ECP_PF_UNCOMPRESSED);
    lua::bind::value(l, "DECRYPT", crypto::DECRYPT);
    lua::bind::value(l, "ENCRYPT", crypto::ENCRYPT);
    lua::bind::value(l, "PADDING_PKCS7", crypto::PADDING_PKCS7);
    lua::bind::value(l, "PADDING_ONE_AND_ZEROS", crypto::PADDING_ONE_AND_ZEROS);
    lua::bind::value(l, "PADDING_ZEROS_AND_LEN", crypto::PADDING_ZEROS_AND_LEN);
    lua::bind::value(l, "PADDING_ZEROS", crypto::PADDING_ZEROS);
    lua::bind::value(l, "PADDING_NONE", crypto::PADDING_NONE);
    lua::bind::value(l, "RSA_PKCS_V15", crypto::RSA_PKCS_V15);
    lua::bind::value(l, "RSA_PKCS_V21", crypto::RSA_PKCS_V21);
    lua::bind::value(l, "MD_NONE", crypto::MD_NONE);
    lua::bind::value(l, "MD_MD5", crypto::MD_MD5);
    lua::bind::value(l, "MD_SHA1", crypto::MD_SHA1);
    lua::bind::value(l, "MD_SHA224", crypto::MD_SHA224);
    lua::bind::value(l, "MD_SHA256", crypto::MD_SHA256);
    lua::bind::value(l, "MD_SHA384", crypto::MD_SHA384);
    lua::bind::value(l, "MD_SHA512", crypto::MD_SHA512);
    lua::bind::value(l, "MD_RIPEMD160", crypto::MD_RIPEMD160);
    return 1;
}
