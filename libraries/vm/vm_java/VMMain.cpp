#include "VMMain.h"
#include <stdio.h>

/*
 * Class:     vmapi4java
 * Method:    sayHello
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_VMMain_sayHello(JNIEnv *env, jobject jobj) {
   printf("++++++++++++++hello,world\n");
}

JNIEXPORT void JNICALL Java_VMMain_apply(JNIEnv *env, jobject obj, jlong p1, jlong p2, jlong p3) {
   printf("++++++%lld %lld %lld \n", p1, p2, p3);
}

/*
 * Class:     VMMain
 * Method:    is_account
 * Signature: (J)Z
 */
JNIEXPORT jboolean JNICALL Java_VMMain_is_1account
  (JNIEnv *, jobject, jlong){
   return true;
}

/*
 * Class:     VMMain
 * Method:    s2n
 * Signature: (Ljava/lang/String;)J
 */
JNIEXPORT jlong JNICALL Java_VMMain_s2n
  (JNIEnv *, jobject, jstring){
   return 0;
}

/*
 * Class:     VMMain
 * Method:    n2s
 * Signature: (J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_VMMain_n2s
  (JNIEnv *env, jobject, jlong){
   return env->NewStringUTF("hello,world");
}

/*
 * Class:     VMMain
 * Method:    action_data_size
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_VMMain_action_1data_1size
  (JNIEnv *, jobject){
   return 0;
}

/*
 * Class:     VMMain
 * Method:    read_action_data
 * Signature: ()[B
 */
JNIEXPORT jbyteArray JNICALL Java_VMMain_read_1action_1data
  (JNIEnv *env, jobject){
   return env->NewByteArray(10);
}

/*
 * Class:     VMMain
 * Method:    require_recipient
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_VMMain_require_1recipient
  (JNIEnv *, jobject, jlong){}

/*
 * Class:     VMMain
 * Method:    require_auth
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_VMMain_require_1auth
  (JNIEnv *, jobject, jlong){}

/*
 * Class:     VMMain
 * Method:    db_store_i64
 * Signature: (JJJJ[B)I
 */
JNIEXPORT jint JNICALL Java_VMMain_db_1store_1i64
  (JNIEnv *, jobject, jlong, jlong, jlong, jlong, jbyteArray){
   return 0;
}

/*
 * Class:     VMMain
 * Method:    db_update_i64
 * Signature: (I[B)V
 */
JNIEXPORT void JNICALL Java_VMMain_db_1update_1i64
  (JNIEnv *, jobject, jint, jbyteArray){
   return;
}

/*
 * Class:     VMMain
 * Method:    db_remove_i64
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_VMMain_db_1remove_1i64
  (JNIEnv *, jobject, jint){

}

/*
 * Class:     VMMain
 * Method:    db_get_i64
 * Signature: (I)[B
 */
JNIEXPORT jbyteArray JNICALL Java_VMMain_db_1get_1i64
  (JNIEnv *env, jobject, jint){
   return env->NewByteArray(10);
}

/*
 * Class:     VMMain
 * Method:    db_next_i64
 * Signature: (I)J
 */
JNIEXPORT jlong JNICALL Java_VMMain_db_1next_1i64
  (JNIEnv *, jobject, jint){
   return 0;
}

/*
 * Class:     VMMain
 * Method:    db_previous_i64
 * Signature: (I)J
 */
JNIEXPORT jlong JNICALL Java_VMMain_db_1previous_1i64
  (JNIEnv *, jobject, jint){
   return 0;
}

/*
 * Class:     VMMain
 * Method:    db_find_i64
 * Signature: (JJJJ)I
 */
JNIEXPORT jint JNICALL Java_VMMain_db_1find_1i64
  (JNIEnv *, jobject, jlong, jlong, jlong, jlong){
   return 0;
}

/*
 * Class:     VMMain
 * Method:    db_lowerbound_i64
 * Signature: (JJJJ)I
 */
JNIEXPORT jint JNICALL Java_VMMain_db_1lowerbound_1i64
  (JNIEnv *, jobject, jlong, jlong, jlong, jlong){
   return 0;
}

/*
 * Class:     VMMain
 * Method:    db_upperbound_i64
 * Signature: (JJJJ)I
 */
JNIEXPORT jint JNICALL Java_VMMain_db_1upperbound_1i64
  (JNIEnv *, jobject, jlong, jlong, jlong, jlong){
   return 0;
}

/*
 * Class:     VMMain
 * Method:    db_end_i64
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_VMMain_db_1end_1i64
  (JNIEnv *, jobject){
   return 0;
}
