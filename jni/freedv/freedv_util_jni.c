/**
 *
 */
JNIEXPORT jfloat JNICALL
Java_org_codec2_demo_Codec2_getSignalNoiseEstimation(JNIEnv* env UNUSED,
        jobject foo UNUSED)
{
    return stats.snr_est;
}

/**
 *
 */
JNIEXPORT jfloat JNICALL
Java_org_codec2_demo_Codec2_getFrequencyOffset(JNIEnv* env UNUSED,
        jobject foo UNUSED)
{
    return stats.foff;
}

/**
 *
 */
JNIEXPORT jfloat JNICALL
Java_org_codec2_demo_Codec2_getTimingOffset(JNIEnv* env UNUSED,
        jobject foo UNUSED)
{
    return stats.rx_timing;
}

/**
 *
 */
JNIEXPORT jfloat JNICALL
Java_org_codec2_demo_Codec2_getClockOffsetPpm(JNIEnv* env UNUSED,
        jobject foo UNUSED)
{
    return stats.clock_offset;
}

/**
 *    COMP   rx_symbols[FDMDV_NSYM]
 * latest received symbols, for scatter plot 
 *
 * FDMDV_NSYM 15
 *
 * typedef struct {
 *   float real;
 *   float imag;
 * } COMP;
 *
 */
JNIEXPORT jfloatArray JNICALL
Java_org_codec2_demo_Codec2_getReceivedSymbols(JNIEnv* env UNUSED,
        jobject foo UNUSED)
{
    jfloatArray outputArray = (*env)->NewFloatArray(env, N8);
    (*env)->SetFloatArrayRegion(env, outputArray, 0, FDMDV_NSYM*2,
            (float *)stats.rx_symbols);
    return outputArray;
}

/**
 *
 */
JNIEXPORT jboolean JNICALL
Java_org_codec2_demo_Codec2_isModemInSync(JNIEnv* env UNUSED,
        jobject foo UNUSED)
{
    return g_state > 0;
}

