/* APatch JNI接口实现 - Android应用程序的原生接口层 */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/* 
 * Copyright (C) 2023 bmax121. All Rights Reserved.
 */

#include <jni.h>
#include <android/log.h>
#include <cstring>

#include "../supercall.h"

// Android日志标签定义
#define LOG_TAG "APatchNative"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

/**
 * @brief 将整数数组填充到Java ArrayList中
 * @details 通过反射机制调用ArrayList的add方法添加Integer对象
 * @param env JNI环境指针
 * @param list Java ArrayList对象
 * @param data 整数数据数组
 * @param count 数组元素数量
 */
static void fillIntArray(JNIEnv *env, jobject list, int *data, int count)
{
    auto cls = env->GetObjectClass(list);                           // 获取ArrayList类
    auto add = env->GetMethodID(cls, "add", "(Ljava/lang/Object;)Z"); // 获取add方法ID
    auto integerCls = env->FindClass("java/lang/Integer");          // 查找Integer类
    auto constructor = env->GetMethodID(integerCls, "<init>", "(I)V"); // 获取构造函数
    
    for (int i = 0; i < count; ++i) {
        // 创建Integer对象并添加到ArrayList
        auto integer = env->NewObject(integerCls, constructor, data[i]);
        env->CallBooleanMethod(list, add, integer);
    }
}

/**
 * @brief 向Java ArrayList中添加单个整数
 * @param env JNI环境指针
 * @param list Java ArrayList对象
 * @param ele 要添加的整数元素
 */
static void addIntToList(JNIEnv *env, jobject list, int ele)
{
    auto cls = env->GetObjectClass(list);                           // 获取ArrayList类
    auto add = env->GetMethodID(cls, "add", "(Ljava/lang/Object;)Z"); // 获取add方法
    auto integerCls = env->FindClass("java/lang/Integer");          // 查找Integer类
    auto constructor = env->GetMethodID(integerCls, "<init>", "(I)V"); // 获取构造函数
    auto integer = env->NewObject(integerCls, constructor, ele);     // 创建Integer对象
    env->CallBooleanMethod(list, add, integer);                     // 添加到列表
}

/**
 * @brief 获取Java List的大小
 * @param env JNI环境指针
 * @param list Java List对象
 * @return 列表中的元素数量
 */
static int getListSize(JNIEnv *env, jobject list)
{
    auto cls = env->GetObjectClass(list);            // 获取List类
    auto size = env->GetMethodID(cls, "size", "()I"); // 获取size方法
    return env->CallIntMethod(list, size);           // 调用size方法获取大小
}

/**
 * @brief 检查KernelPatch是否已准备就绪
 * @param env JNI环境指针
 * @param clz Java类引用
 * @param superKey 超级调用密钥字符串
 * @return 成功返回true，失败返回false
 */
extern "C" JNIEXPORT jboolean JNICALL Java_me_bmax_apatch_Natives_nativeReady(JNIEnv *env, jclass clz, jstring superKey)
{
    if (!superKey) return -EINVAL;  // 检查参数有效性
    
    const char *skey = env->GetStringUTFChars(superKey, NULL);  // 获取UTF-8字符串
    bool rc = sc_ready(skey);  // 调用超级调用检查就绪状态
    env->ReleaseStringUTFChars(superKey, skey);  // 释放字符串资源
    
    return rc;
}

/**
 * @brief 获取KernelPatch版本号
 * @param env JNI环境指针
 * @param clz Java类引用
 * @param superKey 超级调用密钥字符串
 * @return 版本号或错误码
 */
extern "C" JNIEXPORT jint JNICALL Java_me_bmax_apatch_Natives_nativeKernelPatchVersion(JNIEnv *env, jclass clz,
                                                                                       jstring superKey)
{
    if (!superKey) return -EINVAL;  // 参数检查
    
    const char *skey = env->GetStringUTFChars(superKey, NULL);  // 获取密钥字符串
    uint32_t version = sc_kp_ver(skey);  // 调用超级调用获取版本
    env->ReleaseStringUTFChars(superKey, skey);  // 释放资源
    
    return version;
}

/**
 * @brief 执行用户权限提升操作
 * @param env JNI环境指针
 * @param clz Java类引用
 * @param superKey 超级调用密钥字符串
 * @param to_uid 目标用户ID
 * @param scontext SELinux安全上下文
 * @return 操作结果，成功返回0，失败返回负数错误码
 */
extern "C" JNIEXPORT jlong JNICALL Java_me_bmax_apatch_Natives_nativeSu(JNIEnv *env, jclass clz, jstring superKey,
                                                                        jint to_uid, jstring scontext)
{
    if (!superKey) return -EINVAL;  // 检查必要参数
    
    const char *skey = env->GetStringUTFChars(superKey, NULL);  // 获取密钥
    const char *sctx = 0;
    if (scontext) sctx = env->GetStringUTFChars(scontext, NULL);  // 获取安全上下文（可选）
    
    // 构建su配置信息
    struct su_profile profile = { 0 };
    profile.uid = getuid();         // 当前用户ID
    profile.to_uid = (uid_t)to_uid; // 目标用户ID
    if (sctx) strncpy(profile.scontext, sctx, sizeof(profile.scontext) - 1);  // 设置安全上下文
    
    long rc = sc_su(skey, &profile);  // 执行超级调用su操作
    if (rc < 0) LOGE("nativeSu error: %ld\n", rc);  // 记录错误日志
    
    // 释放字符串资源
    env->ReleaseStringUTFChars(superKey, skey);
    if (sctx) env->ReleaseStringUTFChars(scontext, sctx);
    
    return rc;
}

/**
 * @brief 对指定线程执行权限提升操作
 * @param env JNI环境指针
 * @param clz Java类引用
 * @param superKey 超级调用密钥字符串
 * @param tid 目标线程ID
 * @param to_uid 目标用户ID
 * @param scontext SELinux安全上下文
 * @return 操作结果
 */
extern "C" JNIEXPORT jlong JNICALL Java_me_bmax_apatch_Natives_nativeThreadSu(JNIEnv *env, jclass clz, jstring superKey,
                                                                              jint tid, jint to_uid, jstring scontext)
{
    const char *skey = env->GetStringUTFChars(superKey, NULL);  // 获取密钥
    const char *sctx = 0;
    if (scontext) sctx = env->GetStringUTFChars(scontext, NULL);  // 获取安全上下文
    
    // 构建su配置信息
    struct su_profile profile = { 0 };
    profile.uid = getuid();         // 当前用户ID
    profile.to_uid = (uid_t)to_uid; // 目标用户ID
    if (sctx) strncpy(profile.scontext, sctx, sizeof(profile.scontext) - 1);  // 设置安全上下文
    
    long rc = sc_su_task(skey, tid, &profile);  // 对指定任务执行su操作
    
    // 释放资源
    env->ReleaseStringUTFChars(superKey, skey);
    env->ReleaseStringUTFChars(scontext, sctx);
    
    return rc;
}

/**
 * @brief 获取已提权用户的数量
 * @param env JNI环境指针
 * @param clz Java类引用
 * @param superKey 超级调用密钥字符串
 * @return 提权用户数量
 */
extern "C" JNIEXPORT jint JNICALL Java_me_bmax_apatch_Natives_nativeSuNums(JNIEnv *env, jclass clz, jstring superKey)
{
    const char *skey = env->GetStringUTFChars(superKey, NULL);  // 获取密钥
    long rc = sc_su_uid_nums(skey);
    env->ReleaseStringUTFChars(superKey, skey);
    return rc;
}

extern "C" JNIEXPORT jintArray JNICALL Java_me_bmax_apatch_Natives_nativeSuUids(JNIEnv *env, jclass clz,
                                                                                jstring superKey)
{
    const char *skey = env->GetStringUTFChars(superKey, NULL);
    int num = sc_su_uid_nums(skey);
    int uids[num];
    long n = sc_su_allow_uids(skey, (uid_t *)uids, num);
    if (n > 0) {
        jintArray array = env->NewIntArray(num);
        env->SetIntArrayRegion(array, 0, n, uids);
        return array;
    }
    env->ReleaseStringUTFChars(superKey, skey);
    return env->NewIntArray(0);
}

extern "C" JNIEXPORT jobject JNICALL Java_me_bmax_apatch_Natives_nativeSuProfile(JNIEnv *env, jclass clz,
                                                                                 jstring superKey, jint uid)
{
    const char *skey = env->GetStringUTFChars(superKey, NULL);
    struct su_profile profile = { 0 };
    long rc = sc_su_uid_profile(skey, (uid_t)uid, &profile);
    if (rc < 0) {
        LOGE("nativeSuProfile error: %ld\n", rc);
        env->ReleaseStringUTFChars(superKey, skey);
        return nullptr;
    }
    jclass cls = env->FindClass("me/bmax/apatch/Natives$Profile");
    jmethodID constructor = env->GetMethodID(cls, "<init>", "()V");
    jfieldID uidField = env->GetFieldID(cls, "uid", "I");
    jfieldID toUidField = env->GetFieldID(cls, "toUid", "I");
    jfieldID scontextFild = env->GetFieldID(cls, "scontext", "Ljava/lang/String;");

    jobject obj = env->NewObject(cls, constructor);
    env->SetIntField(obj, uidField, profile.uid);
    env->SetIntField(obj, toUidField, profile.to_uid);
    env->SetObjectField(obj, scontextFild, env->NewStringUTF(profile.scontext));

    return obj;
}

extern "C" JNIEXPORT jlong JNICALL Java_me_bmax_apatch_Natives_nativeLoadKernelPatchModule(JNIEnv *env, jclass clz,
                                                                                           jstring superKey,
                                                                                           jstring modulePath,
                                                                                           jstring jargs)
{
    const char *skey = env->GetStringUTFChars(superKey, NULL);
    const char *path = env->GetStringUTFChars(modulePath, NULL);
    const char *args = env->GetStringUTFChars(jargs, NULL);
    long rc = sc_kpm_load(skey, path, args, 0);
    if (rc < 0) LOGE("nativeLoadKernelPatchModule error: %ld\n", rc);
    env->ReleaseStringUTFChars(superKey, skey);
    env->ReleaseStringUTFChars(modulePath, path);
    env->ReleaseStringUTFChars(jargs, args);
    return rc;
}

extern "C" JNIEXPORT jobject JNICALL Java_me_bmax_apatch_Natives_nativeControlKernelPatchModule(JNIEnv *env, jclass clz,
                                                                                                jstring superKey,
                                                                                                jstring modName,
                                                                                                jstring jctlargs)
{
    const char *skey = env->GetStringUTFChars(superKey, NULL);
    const char *name = env->GetStringUTFChars(modName, NULL);
    const char *ctlargs = env->GetStringUTFChars(jctlargs, NULL);

    char buf[4096] = { '\0' };
    long rc = sc_kpm_control(skey, name, ctlargs, buf, sizeof(buf));
    if (rc < 0) LOGE("nativeControlKernelPatchModule error: %ld\n", rc);

    jclass cls = env->FindClass("me/bmax/apatch/Natives$KPMCtlRes");
    jmethodID constructor = env->GetMethodID(cls, "<init>", "()V");
    jfieldID rcField = env->GetFieldID(cls, "rc", "J");
    jfieldID outMsg = env->GetFieldID(cls, "outMsg", "Ljava/lang/String;");

    jobject obj = env->NewObject(cls, constructor);
    env->SetLongField(obj, rcField, rc);
    env->SetObjectField(obj, outMsg, env->NewStringUTF(buf));

    env->ReleaseStringUTFChars(superKey, skey);
    env->ReleaseStringUTFChars(modName, name);
    env->ReleaseStringUTFChars(jctlargs, ctlargs);
    return obj;
}

extern "C" JNIEXPORT jlong JNICALL Java_me_bmax_apatch_Natives_nativeUnloadKernelPatchModule(JNIEnv *env, jclass clz,
                                                                                             jstring superKey,
                                                                                             jstring modName)
{
    const char *skey = env->GetStringUTFChars(superKey, NULL);
    const char *name = env->GetStringUTFChars(modName, NULL);
    long rc = sc_kpm_unload(skey, name, 0);
    if (rc < 0) LOGE("nativeUnloadKernelPatchModule error: %ld\n", rc);
    env->ReleaseStringUTFChars(superKey, skey);
    env->ReleaseStringUTFChars(modName, name);
    return rc;
}

extern "C" JNIEXPORT jlong JNICALL Java_me_bmax_apatch_Natives_nativeKernelPatchModuleNum(JNIEnv *env, jclass clz,
                                                                                          jstring superKey)
{
    const char *skey = env->GetStringUTFChars(superKey, NULL);
    long rc = sc_kpm_nums(skey);
    if (rc < 0) LOGE("nativeKernelPatchModuleNum error: %ld\n", rc);

    env->ReleaseStringUTFChars(superKey, skey);
    return rc;
}

extern "C" JNIEXPORT jstring JNICALL Java_me_bmax_apatch_Natives_nativeKernelPatchModuleList(JNIEnv *env, jclass clz,
                                                                                             jstring superKey)
{
    const char *skey = env->GetStringUTFChars(superKey, NULL);
    long rc = sc_kpm_nums(skey);
    char buf[4096] = { '\0' };
    rc = sc_kpm_list(skey, buf, sizeof(buf));
    if (rc < 0) LOGE("nativeKernelPatchModuleList error: %ld\n", rc);

    env->ReleaseStringUTFChars(superKey, skey);
    return env->NewStringUTF(buf);
}

extern "C" JNIEXPORT jstring JNICALL Java_me_bmax_apatch_Natives_nativeKernelPatchModuleInfo(JNIEnv *env, jclass clz,
                                                                                             jstring superKey,
                                                                                             jstring modName)
{
    const char *skey = env->GetStringUTFChars(superKey, NULL);
    const char *name = env->GetStringUTFChars(modName, NULL);
    char buf[1024] = { '\0' };
    long rc = sc_kpm_info(skey, name, buf, sizeof(buf));
    if (rc < 0) LOGE("nativeKernelPatchModuleInfo error: %ld\n", rc);
    env->ReleaseStringUTFChars(superKey, skey);
    env->ReleaseStringUTFChars(modName, name);
    return env->NewStringUTF(buf);
}

extern "C" JNIEXPORT jlong JNICALL Java_me_bmax_apatch_Natives_nativeGrantSu(JNIEnv *env, jclass clz, jstring superKey,
                                                                             jint uid, jint to_uid, jstring scontext)
{
    const char *skey = env->GetStringUTFChars(superKey, NULL);
    const char *sctx = env->GetStringUTFChars(scontext, NULL);
    struct su_profile profile = { 0 };
    profile.uid = uid;
    profile.to_uid = to_uid;
    if (sctx) strncpy(profile.scontext, sctx, sizeof(profile.scontext) - 1);
    long rc = sc_su_grant_uid(skey, uid, &profile);
    env->ReleaseStringUTFChars(superKey, skey);
    env->ReleaseStringUTFChars(scontext, sctx);
    return rc;
}

extern "C" JNIEXPORT jlong JNICALL Java_me_bmax_apatch_Natives_nativeRevokeSu(JNIEnv *env, jclass clz, jstring superKey,
                                                                              jint uid)
{
    const char *skey = env->GetStringUTFChars(superKey, NULL);
    long rc = sc_su_revoke_uid(skey, (uid_t)uid);
    env->ReleaseStringUTFChars(superKey, skey);
    return rc;
}

extern "C" JNIEXPORT jstring JNICALL Java_me_bmax_apatch_Natives_nativeSuPath(JNIEnv *env, jclass clz, jstring superKey)
{
    const char *skey = env->GetStringUTFChars(superKey, NULL);
    char buf[SU_PATH_MAX_LEN] = { '\0' };
    long rc = sc_su_get_path(skey, buf, sizeof(buf));
    env->ReleaseStringUTFChars(superKey, skey);
    return env->NewStringUTF(buf);
}

extern "C" JNIEXPORT jboolean JNICALL Java_me_bmax_apatch_Natives_nativeResetSuPath(JNIEnv *env, jclass clz,
                                                                                    jstring superKey, jstring jpath)
{
    const char *skey = env->GetStringUTFChars(superKey, NULL);
    const char *path = env->GetStringUTFChars(jpath, NULL);
    long rc = sc_su_reset_path(skey, path);
    env->ReleaseStringUTFChars(superKey, skey);
    env->ReleaseStringUTFChars(jpath, path);
    return rc == 0;
}
