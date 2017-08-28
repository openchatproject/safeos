#ifndef __HELLO_H
#define __HELLO_H
	int get_info_(char *result,int length);
	int get_block_(int id,char *result,int length);
    int get_account_(char* name,char *result,int length);
    int create_account_( char* creator_,char* newaccount_,char* owner_key_,char* active_key_,char *ts_result,int length);
	int create_key_(char *pub_,int pub_length,char *priv_,int priv_length);
    int get_transaction_(char *id,char* result,int length);
    int transfer_(char *sender_,char* recipient_,int amount,char *result,int length);
    int setcode_(char *account_,char *wast_file,char *abi_file,char *ts_buffer,int length);
#endif


