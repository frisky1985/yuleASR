/**
 * @file test_mqtt_certmgr.c
 * @brief Mqtt_CertMgr Unit Tests
 *
 * Mqtt_CertMgr.c is compiled directly against the REAL mbedtls headers; all
 * mbedtls_x509_crt_* functions are replaced by the configurable stubs in
 * mbedtls_stubs.c. "FAKE" prefixed certificate data parses into a template
 * certificate whose subject/issuer chains, serial and validity dates are
 * controlled by the test (see mbedtls_stubs.c).
 *
 * NOTE (product behaviour, recorded not fixed): CertMgr time checks compare
 * seconds (1700000000) against year*31536000, so a certificate is VALID only
 * when valid_from.year <= 53 and valid_to.year == 54. Normal certificate
 * years (>= 55) always report NOT_YET_VALID. Tests drive both regimes.
 */

// @tests src/bsw/services/mqtt/src/Mqtt_CertMgr.c  @tests src/bsw/services/mqtt/include/Mqtt_CertMgr.h

#include "unity.h"
#include "Mqtt_CertMgr.h"
#include "mbedtls/x509_crt.h"
#include <string.h>
#include <stdio.h>

/* ---- mbedtls stub knobs (defined in mbedtls_stubs.c) ---- */
extern int stub_x509_parse_mode;
extern int stub_cert_valid_from_year;
extern int stub_cert_valid_to_year;
extern int stub_x509_verify_ret;
extern unsigned int stub_x509_verify_flags;
extern int stub_parse_calls;
extern void stub_all_reset(void);

/* Test certificate data: must start with "FAKE" so the stub parses it. */
static const char fakePem[] = "FAKE-CERTIFICATE-BODY";
/* Data the stub refuses to parse (no "FAKE" magic). */
static const uint8 badData[] = { 0xDE, 0xAD, 0xBE, 0xEF };

/* Callback recorders */
static int rec_expiry_calls;
static char rec_expiry_alias[32];
static unsigned int rec_expiry_days;

static void recordExpiryCb(const char* alias, unsigned int daysUntilExpiry) {
    rec_expiry_calls++;
    if (alias != NULL_PTR) {
        strncpy(rec_expiry_alias, alias, sizeof(rec_expiry_alias) - 1U);
        rec_expiry_alias[sizeof(rec_expiry_alias) - 1U] = '\0';
    }
    rec_expiry_days = daysUntilExpiry;
}

static int rec_reload_calls;
static char rec_reload_alias[32];
static Mqtt_ReturnType rec_reload_result;

static void recordReloadCb(const char* alias, Mqtt_ReturnType result) {
    rec_reload_calls++;
    if (alias != NULL_PTR) {
        strncpy(rec_reload_alias, alias, sizeof(rec_reload_alias) - 1U);
        rec_reload_alias[sizeof(rec_reload_alias) - 1U] = '\0';
    }
    rec_reload_result = result;
}

static void resetAll(void) {
    stub_all_reset();
    stub_x509_verify_ret = 0;
    stub_x509_verify_flags = 0U;
}

void setUp(void) {
    resetAll();
    stub_cert_valid_from_year = 2023;
    stub_cert_valid_to_year = 2033;
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK,
        Mqtt_CertMgr_Init(NULL_PTR));
    stub_cert_valid_from_year = 2023;
    stub_cert_valid_to_year = 2033;
}

void tearDown(void) {
    Mqtt_CertMgr_DeInit();
    resetAll();
}

static Mqtt_CertEntryType makeEntry(const char* alias, Mqtt_CertType type) {
    Mqtt_CertEntryType entry;
    memset(&entry, 0, sizeof(entry));
    entry.type = type;
    strncpy(entry.alias, alias, sizeof(entry.alias) - 1U);
    entry.data = (uint8*)fakePem;
    entry.dataLen = (uint32)strlen(fakePem) + 1U;
    entry.storage = MQTT_CERT_STORAGE_RAM;
    entry.isLoaded = TRUE;
    return entry;
}

static void addCaRoot(void) {
    Mqtt_CertEntryType entry = makeEntry("ca-root", MQTT_CERT_TYPE_CA_ROOT);
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_CertMgr_AddCert(&entry));
}

static void defaultCfg(Mqtt_CertMgrConfigType* cfg) {
    memset(cfg, 0, sizeof(*cfg));
    cfg->autoReload = TRUE;
    cfg->checkIntervalMs = 0U;   /* forces MainFunction to run its scan */
    cfg->strictValidation = TRUE;
    cfg->expiryWarningDays = 30U;
}

/*============================================================================
 * Init / DeInit
 *============================================================================*/

void test_CertMgr_Init_NullConfig_ShouldUseDefaults(void) {
    /* setUp already inited with NULL; just verify it is operational. */
    TEST_ASSERT_EQUAL_UINT8(0U, Mqtt_CertMgr_GetCertCount());
}

void test_CertMgr_Init_CustomConfig_ShouldBeIdempotent(void) {
    Mqtt_CertMgrConfigType cfg;
    defaultCfg(&cfg);
    /* NOTE (actual behaviour): a second Init is a no-op that returns MQTT_OK;
     * the new config is NOT applied. Prove it behaviourally: with the custom
     * config's checkIntervalMs=0 the scan still runs after the 2nd Init. */
    Mqtt_CertMgr_DeInit();
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_CertMgr_Init(&cfg));
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_CertMgr_Init(NULL_PTR));
    stub_cert_valid_from_year = 0;
    stub_cert_valid_to_year = 0;
    {
        Mqtt_CertEntryType entry = makeEntry("soon", MQTT_CERT_TYPE_SERVER);
        TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_CertMgr_AddCert(&entry));
    }
    rec_expiry_calls = 0;
    Mqtt_CertMgr_SetExpiryCallback(&recordExpiryCb);
    Mqtt_CertMgr_MainFunction();
    TEST_ASSERT_EQUAL_INT(1, rec_expiry_calls); /* custom interval still active */
}

void test_CertMgr_DeInit_ShouldClearStore(void) {
    addCaRoot();
    TEST_ASSERT_EQUAL_UINT8(1U, Mqtt_CertMgr_GetCertCount());
    Mqtt_CertMgr_DeInit();
    TEST_ASSERT_EQUAL_UINT8(0U, Mqtt_CertMgr_GetCertCount());
}

/*============================================================================
 * AddCert / RemoveCert / GetCert / UpdateCert
 *============================================================================*/

void test_CertMgr_AddCert_WithoutInit_ShouldFail(void) {
    Mqtt_CertEntryType entry = makeEntry("x", MQTT_CERT_TYPE_CA_ROOT);
    Mqtt_CertMgr_DeInit();
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_CertMgr_AddCert(&entry));
}

void test_CertMgr_AddCert_NullEntry_ShouldFail(void) {
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_CertMgr_AddCert(NULL_PTR));
}

void test_CertMgr_AddCert_TooLarge_ShouldFail(void) {
    static uint8 bigData[33000];
    Mqtt_CertEntryType entry = makeEntry("too-big", MQTT_CERT_TYPE_CA_ROOT);
    entry.data = bigData;
    /* NOTE (actual behaviour): there is no per-cert MAX_CERT_SIZE check --
     * AddCert only rejects when the shared 32 KiB pool would overflow. */
    entry.dataLen = sizeof(bigData); /* > CERTMGR_CERT_DATA_SIZE (8*4096) */
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_CertMgr_AddCert(&entry));
    TEST_ASSERT_EQUAL_UINT8(0U, Mqtt_CertMgr_GetCertCount());
}

void test_CertMgr_AddCert_StoreFull_ShouldFail(void) {
    char alias[16];
    int i;
    Mqtt_CertEntryType entry;
    for (i = 0; i < 8; i++) {
        snprintf(alias, sizeof(alias), "cert-%d", i);
        entry = makeEntry(alias, MQTT_CERT_TYPE_CA_ROOT);
        TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_CertMgr_AddCert(&entry));
    }
    entry = makeEntry("cert-overflow", MQTT_CERT_TYPE_CA_ROOT);
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_CertMgr_AddCert(&entry));
    TEST_ASSERT_EQUAL_UINT8(8U, Mqtt_CertMgr_GetCertCount());
}

void test_CertMgr_AddCert_DuplicateAlias_ShouldUpdateInPlace(void) {
    Mqtt_CertEntryType entry = makeEntry("ca-root", MQTT_CERT_TYPE_CA_ROOT);
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_CertMgr_AddCert(&entry));
    entry = makeEntry("ca-root", MQTT_CERT_TYPE_CA_ROOT);
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_CertMgr_AddCert(&entry));
    TEST_ASSERT_EQUAL_UINT8(1U, Mqtt_CertMgr_GetCertCount());
}

void test_CertMgr_AddCert_ShouldParseSubjectAndExtensions(void) {
    Mqtt_CertEntryType entry = makeEntry("ca-root", MQTT_CERT_TYPE_CA_ROOT);
    Mqtt_CertEntryType stored;
    addCaRoot();
    memset(&stored, 0, sizeof(stored));
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_CertMgr_GetCert("ca-root", &stored));
    /* PRODUCT DEFECT (recorded, not fixed): MQTT_OID_CMP returns 1 on match,
     * but CertMgr_ParseSubject tests "== 0U", i.e. it writes on NON-match.
     * Every non-CN node therefore overwrites commonName (the last, email node
     * wins) and the CN node's value lands in organization. All other fields
     * stay at their defaults. */
    TEST_ASSERT_EQUAL_STRING("bsw@yuletech.example", stored.subject.commonName);
    TEST_ASSERT_EQUAL_STRING("yule-client", stored.subject.organization);
    TEST_ASSERT_EQUAL_STRING("", stored.subject.organizationalUnit);
    TEST_ASSERT_EQUAL_STRING("", stored.subject.country);
    TEST_ASSERT_EQUAL_STRING("", stored.subject.state);
    TEST_ASSERT_EQUAL_STRING("", stored.subject.locality);
    TEST_ASSERT_EQUAL_STRING("", stored.subject.email);
    /* Issuer chain is two CN-OID nodes carrying "Test Issuer CA": same defect
     * routes the value into organization; commonName keeps its default. */
    TEST_ASSERT_EQUAL_STRING("Unknown", stored.issuer.commonName);
    TEST_ASSERT_EQUAL_STRING("Test Issuer CA", stored.issuer.organization);
    /* Template: serial byte 0xAB, CA flag set, keyUsage 0x86. */
    TEST_ASSERT_EQUAL_UINT32(0xABU, stored.extensions.serialNumber);
    TEST_ASSERT_EQUAL_UINT8(1U, (uint8)stored.extensions.isCA);
    TEST_ASSERT_EQUAL_UINT8(0x86U, stored.extensions.keyUsage);
    /* notBefore/notAfter are derived from the template years. */
    TEST_ASSERT_EQUAL_UINT32(2023U * 31536000U, stored.extensions.notBefore);
    TEST_ASSERT_EQUAL_UINT32(2033U * 31536000U, stored.extensions.notAfter);
    /* Data was copied into the internal pool (same length, not same pointer). */
    TEST_ASSERT_EQUAL_UINT32((uint32)strlen(fakePem) + 1U, stored.dataLen);
    TEST_ASSERT_TRUE(stored.data != entry.data);
    TEST_ASSERT_EQUAL_MEMORY(fakePem, stored.data, stored.dataLen);
}

void test_CertMgr_AddCert_UnparseableData_ShouldUseDefaults(void) {
    /* Data that the stub refuses to parse -> "Unknown" defaults. */
    static const uint8 badData[] = { 0xDE, 0xAD, 0xBE, 0xEF };
    Mqtt_CertEntryType entry = makeEntry("bad", MQTT_CERT_TYPE_CA_ROOT);
    Mqtt_CertEntryType stored;
    entry.data = badData;
    entry.dataLen = sizeof(badData);
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_CertMgr_AddCert(&entry));
    memset(&stored, 0, sizeof(stored));
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_CertMgr_GetCert("bad", &stored));
    TEST_ASSERT_EQUAL_STRING("Unknown", stored.subject.commonName);
    TEST_ASSERT_EQUAL_UINT32(1700000000U, stored.extensions.notBefore);
    TEST_ASSERT_EQUAL_UINT32(1700000000U + 31536000U, stored.extensions.notAfter);
}

void test_CertMgr_RemoveCert_ShouldFreeSlot(void) {
    Mqtt_CertEntryType entry = makeEntry("tmp", MQTT_CERT_TYPE_SERVER);
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_CertMgr_AddCert(&entry));
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_CertMgr_RemoveCert("tmp"));
    TEST_ASSERT_EQUAL_UINT8(0U, Mqtt_CertMgr_GetCertCount());
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_CertMgr_RemoveCert("tmp"));
}

void test_CertMgr_RemoveCert_ParameterChecks_ShouldFail(void) {
    addCaRoot();
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_CertMgr_RemoveCert(NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_CertMgr_RemoveCert("missing"));
    Mqtt_CertMgr_DeInit();
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_CertMgr_RemoveCert("ca-root"));
}

void test_CertMgr_GetCert_ParameterChecks_ShouldFail(void) {
    Mqtt_CertEntryType stored;
    addCaRoot();
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_CertMgr_GetCert(NULL_PTR, &stored));
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_CertMgr_GetCert("ca-root", NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_CertMgr_GetCert("missing", &stored));
}

void test_CertMgr_UpdateCert_ShouldReplaceDataAndNotify(void) {
    static const char newPem[] = "FAKE-RENEWED-CERT";
    Mqtt_CertEntryType stored;
    rec_reload_calls = 0;
    addCaRoot();
    Mqtt_CertMgr_SetReloadCallback(&recordReloadCb);
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK,
        Mqtt_CertMgr_UpdateCert("ca-root", (const uint8*)newPem, (uint32)strlen(newPem) + 1U));
    TEST_ASSERT_EQUAL_INT(1, rec_reload_calls);
    TEST_ASSERT_EQUAL_STRING("ca-root", rec_reload_alias);
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, rec_reload_result);
    memset(&stored, 0, sizeof(stored));
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_CertMgr_GetCert("ca-root", &stored));
    TEST_ASSERT_EQUAL_MEMORY(newPem, stored.data, strlen(newPem) + 1U);
}

void test_CertMgr_UpdateCert_ParameterChecks_ShouldFail(void) {
    static const char newPem[] = "FAKE-RENEWED-CERT";
    addCaRoot();
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_CertMgr_UpdateCert(NULL_PTR, (const uint8*)newPem, 5U));
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_CertMgr_UpdateCert("ca-root", NULL_PTR, 5U));
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_CertMgr_UpdateCert("missing", (const uint8*)newPem, 5U));
}

/*============================================================================
 * Load helpers for TLS
 *============================================================================*/

void test_CertMgr_LoadCertForTls_ShouldExposePooledData(void) {
    Mqtt_CertificateType cert;
    addCaRoot();
    memset(&cert, 0, sizeof(cert));
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_CertMgr_LoadCertForTls("ca-root", &cert));
    TEST_ASSERT_TRUE(cert.data != NULL_PTR);
    TEST_ASSERT_EQUAL_UINT32((uint32)strlen(fakePem) + 1U, cert.length);
    TEST_ASSERT_EQUAL_UINT8(MQTT_CERT_FORMAT_PEM, (uint8)cert.format);
    TEST_ASSERT_EQUAL_UINT8(MQTT_CERT_CACHE_MEMORY, (uint8)cert.cache);
}

void test_CertMgr_LoadCertForTls_ParameterChecks_ShouldFail(void) {
    Mqtt_CertificateType cert;
    addCaRoot();
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_CertMgr_LoadCertForTls(NULL_PTR, &cert));
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_CertMgr_LoadCertForTls("ca-root", NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_CertMgr_LoadCertForTls("missing", &cert));
}

void test_CertMgr_LoadKeyForTls_ShouldExposeKeyData(void) {
    Mqtt_PrivateKeyType key;
    Mqtt_CertEntryType entry = makeEntry("client-key", MQTT_CERT_TYPE_CLIENT);
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_CertMgr_AddCert(&entry));
    memset(&key, 0, sizeof(key));
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_CertMgr_LoadKeyForTls("client-key", "pw", &key));
    TEST_ASSERT_TRUE(key.data != NULL_PTR);
    TEST_ASSERT_EQUAL_UINT32((uint32)strlen(fakePem) + 1U, key.length);
    TEST_ASSERT_EQUAL_STRING("pw", key.password);
}

void test_CertMgr_LoadKeyForTls_MissingAlias_ShouldFail(void) {
    Mqtt_PrivateKeyType key;
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_CertMgr_LoadKeyForTls("missing", NULL_PTR, &key));
}

void test_CertMgr_LoadTrustStore_ByNameAndWildcard(void) {
    Mqtt_TrustStoreType ts;
    addCaRoot();
    memset(&ts, 0, sizeof(ts));
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_CertMgr_LoadTrustStore("ca-root", &ts));
    TEST_ASSERT_TRUE(ts.caCert != NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(MQTT_CERT_FORMAT_PEM, (uint8)ts.caFormat);
    /* Wildcard: first CA_ROOT found. */
    memset(&ts, 0, sizeof(ts));
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_CertMgr_LoadTrustStore("*", &ts));
    TEST_ASSERT_TRUE(ts.caCert != NULL_PTR);
}

void test_CertMgr_LoadTrustStore_NoCaAvailable_ShouldFail(void) {
    Mqtt_TrustStoreType ts;
    Mqtt_CertEntryType entry = makeEntry("only-client", MQTT_CERT_TYPE_CLIENT);
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_CertMgr_AddCert(&entry));
    memset(&ts, 0, sizeof(ts));
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_CertMgr_LoadTrustStore("*", &ts));
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_CertMgr_LoadTrustStore("missing", &ts));
}

/*============================================================================
 * Validation
 *============================================================================*/

void test_CertMgr_ValidateCert_ParameterChecks_ShouldFail(void) {
    Mqtt_CertStatusType status = MQTT_CERT_STATUS_VALID;
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_CertMgr_ValidateCert(NULL_PTR, &status));
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_CertMgr_ValidateCert("ca-root", NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_CertMgr_ValidateCert("missing", &status));
}

void test_CertMgr_ValidateCert_ParseFailure_ShouldReportInvalidFormat(void) {
    Mqtt_CertStatusType status = MQTT_CERT_STATUS_VALID;
    Mqtt_CertEntryType entry = makeEntry("bad", MQTT_CERT_TYPE_CA_ROOT);
    entry.data = badData;
    entry.dataLen = sizeof(badData);
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_CertMgr_AddCert(&entry));
    /* NOTE: validation re-parses the stored bytes; a parse failure is
     * reported as INVALID_FORMAT with MQTT_OK as the API result. */
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_CertMgr_ValidateCert("bad", &status));
    TEST_ASSERT_EQUAL_UINT8(MQTT_CERT_STATUS_INVALID_FORMAT, status);
}

void test_CertMgr_ValidateCert_WithoutInit_ShouldFail(void) {
    Mqtt_CertStatusType status;
    Mqtt_CertMgr_DeInit();
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_CertMgr_ValidateCert("x", &status));
}

void test_CertMgr_ValidateCert_UnknownAlias_ShouldFail(void) {
    Mqtt_CertStatusType status;
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_CertMgr_ValidateCert("missing", &status));
}

void test_CertMgr_ValidateCert_NotYetValidYears_ShouldReportNotYetValid(void) {
    Mqtt_CertStatusType status = MQTT_CERT_STATUS_VALID;
    Mqtt_CertEntryType entry;
    stub_cert_valid_from_year = 2023;
    stub_cert_valid_to_year = 2033;
    entry = makeEntry("n", MQTT_CERT_TYPE_CA_ROOT);
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_CertMgr_AddCert(&entry));
    /* NOTE (actual behaviour): currentTime (1700000000s) is compared against
     * year*31536000, so any normal certificate year (>= 55) is "not yet
     * valid". The year=2023 template triggers this branch. */
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_CertMgr_ValidateCert("n", &status));
    TEST_ASSERT_EQUAL_UINT8(MQTT_CERT_STATUS_NOT_YET_VALID, status);
}

void test_CertMgr_ValidateCert_ExpiredYears_ShouldReportExpired(void) {
    Mqtt_CertStatusType status = MQTT_CERT_STATUS_VALID;
    Mqtt_CertEntryType entry;
    stub_cert_valid_from_year = 0;
    stub_cert_valid_to_year = 0;
    entry = makeEntry("n", MQTT_CERT_TYPE_CA_ROOT);
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_CertMgr_AddCert(&entry));
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_CertMgr_ValidateCert("n", &status));
    TEST_ASSERT_EQUAL_UINT8(MQTT_CERT_STATUS_EXPIRED, status);
}

void test_CertMgr_ValidateCert_WindowYears_ShouldReachVerifyBranches(void) {
    /* from.year=0, to.year=54: 0 <= currentTime(1.7e9) <= 54*31536000
     * -> time checks pass and the mbedTLS verify stub drives the status. */
    Mqtt_CertStatusType status;
    Mqtt_CertEntryType entry;
    stub_cert_valid_from_year = 0;
    stub_cert_valid_to_year = 54;
    entry = makeEntry("v", MQTT_CERT_TYPE_CA_ROOT);

    stub_x509_verify_ret = 0;
    stub_x509_verify_flags = 0U;
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_CertMgr_AddCert(&entry));
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_CertMgr_ValidateCert("v", &status));
    TEST_ASSERT_EQUAL_UINT8(MQTT_CERT_STATUS_VALID, status);

    /* NOTE (actual behaviour): the flags mapping is only consulted when the
     * verify call itself fails (ret != 0); a zero return means VALID no matter
     * what flags are reported. Drive the failure cases with ret = -1. */
    stub_x509_verify_ret = -1;
    stub_x509_verify_flags = (unsigned int)MBEDTLS_X509_BADCERT_EXPIRED;
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_CertMgr_ValidateCert("v", &status));
    TEST_ASSERT_EQUAL_UINT8(MQTT_CERT_STATUS_EXPIRED, status);

    stub_x509_verify_flags = (unsigned int)MBEDTLS_X509_BADCERT_REVOKED;
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_CertMgr_ValidateCert("v", &status));
    TEST_ASSERT_EQUAL_UINT8(MQTT_CERT_STATUS_REVOKED, status);

    stub_x509_verify_flags = (unsigned int)MBEDTLS_X509_BADCERT_NOT_TRUSTED;
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_CertMgr_ValidateCert("v", &status));
    TEST_ASSERT_EQUAL_UINT8(MQTT_CERT_STATUS_UNTRUSTED, status);

    stub_x509_verify_flags = 0x1000U; /* unmapped flag -> signature failure */
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_CertMgr_ValidateCert("v", &status));
    TEST_ASSERT_EQUAL_UINT8(MQTT_CERT_STATUS_SIGNATURE_FAILED, status);
}

void test_CertMgr_ValidateCertChain_WithCa_ShouldVerify(void) {
    boolean isValid = FALSE;
    Mqtt_CertEntryType ca = makeEntry("ca-root", MQTT_CERT_TYPE_CA_ROOT);
    Mqtt_CertEntryType server = makeEntry("server", MQTT_CERT_TYPE_SERVER);
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_CertMgr_AddCert(&ca));
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_CertMgr_AddCert(&server));
    stub_x509_verify_ret = 0;
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_CertMgr_ValidateCertChain("server", "ca-root", &isValid));
    TEST_ASSERT_EQUAL_UINT8(1U, (uint8)isValid);
    /* verify failure -> chain invalid, API still OK. */
    stub_x509_verify_ret = -1;
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_CertMgr_ValidateCertChain("server", "ca-root", &isValid));
    TEST_ASSERT_EQUAL_UINT8(0U, (uint8)isValid);
}

void test_CertMgr_ValidateCertChain_ParseFailure_ShouldReportInvalid(void) {
    boolean isValid = TRUE;
    Mqtt_CertEntryType server = makeEntry("server", MQTT_CERT_TYPE_SERVER);
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_CertMgr_AddCert(&server));
    stub_x509_parse_mode = 2;
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_CertMgr_ValidateCertChain("server", NULL_PTR, &isValid));
    TEST_ASSERT_EQUAL_UINT8(0U, (uint8)isValid);
}

void test_CertMgr_ValidateCertChain_ParameterChecks_ShouldFail(void) {
    boolean isValid;
    Mqtt_CertEntryType server = makeEntry("server", MQTT_CERT_TYPE_SERVER);
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_CertMgr_ValidateCertChain(NULL_PTR, NULL_PTR, &isValid));
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_CertMgr_ValidateCertChain("server", NULL_PTR, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_CertMgr_ValidateCertChain("missing", NULL_PTR, &isValid));
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_CertMgr_AddCert(&server));
    /* NOTE (actual behaviour): an unknown caAlias is tolerated -- the CA load
     * is skipped and verification proceeds against an empty CA chain, so the
     * result comes straight from the verify stub (ret==0 -> isValid=TRUE). */
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK,
        Mqtt_CertMgr_ValidateCertChain("server", "missing-ca", &isValid));
    TEST_ASSERT_EQUAL_UINT8(1U, (uint8)isValid);
}

void test_CertMgr_CheckExpiry_ShouldDetectExpiringCertificates(void) {
    boolean expiring = FALSE;
    Mqtt_CertEntryType entry;
    stub_cert_valid_from_year = 0;
    stub_cert_valid_to_year = 0; /* expireTime = 0 < warningTime -> soon */
    entry = makeEntry("soon", MQTT_CERT_TYPE_SERVER);
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_CertMgr_AddCert(&entry));
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_CertMgr_CheckExpiry("soon", 30U, &expiring));
    TEST_ASSERT_EQUAL_UINT8(1U, (uint8)expiring);

    stub_cert_valid_from_year = 2023;
    stub_cert_valid_to_year = 2033; /* expireTime >> warningTime */
    entry = makeEntry("late", MQTT_CERT_TYPE_SERVER);
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_CertMgr_AddCert(&entry));
    expiring = TRUE;
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_CertMgr_CheckExpiry("late", 30U, &expiring));
    TEST_ASSERT_EQUAL_UINT8(0U, (uint8)expiring);
}

void test_CertMgr_CheckExpiry_ParameterChecks_ShouldFail(void) {
    boolean expiring;
    Mqtt_CertEntryType entry = makeEntry("n", MQTT_CERT_TYPE_SERVER);
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_CertMgr_CheckExpiry(NULL_PTR, 30U, &expiring));
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_CertMgr_CheckExpiry("n", 30U, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_CertMgr_CheckExpiry("missing", 30U, &expiring));
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_CertMgr_AddCert(&entry));
    stub_x509_parse_mode = 2;
    /* NOTE: parse failure inside CheckExpiry is reported as NOT_OK. */
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_CertMgr_CheckExpiry("n", 30U, &expiring));
}

/*============================================================================
 * Listing
 *============================================================================*/

void test_CertMgr_GetCertCount_WithoutInit_ShouldBeZero(void) {
    Mqtt_CertMgr_DeInit();
    TEST_ASSERT_EQUAL_UINT8(0U, Mqtt_CertMgr_GetCertCount());
}

void test_CertMgr_GetCertAliasByIndex_ShouldEnumerate(void) {
    char alias[32];
    Mqtt_CertEntryType a = makeEntry("ca-root", MQTT_CERT_TYPE_CA_ROOT);
    Mqtt_CertEntryType b = makeEntry("server", MQTT_CERT_TYPE_SERVER);
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_CertMgr_AddCert(&a));
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_CertMgr_AddCert(&b));
    memset(alias, 0, sizeof(alias));
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_CertMgr_GetCertAliasByIndex(0U, alias, sizeof(alias)));
    TEST_ASSERT_EQUAL_STRING("ca-root", alias);
    memset(alias, 0, sizeof(alias));
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_CertMgr_GetCertAliasByIndex(1U, alias, sizeof(alias)));
    TEST_ASSERT_EQUAL_STRING("server", alias);
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_CertMgr_GetCertAliasByIndex(2U, alias, sizeof(alias)));
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_CertMgr_GetCertAliasByIndex(0U, NULL_PTR, sizeof(alias)));
}

void test_CertMgr_FindCertByType_ShouldFindAndReportMissing(void) {
    char alias[32];
    Mqtt_CertEntryType a = makeEntry("ca-root", MQTT_CERT_TYPE_CA_ROOT);
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_CertMgr_AddCert(&a));
    memset(alias, 0, sizeof(alias));
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_CertMgr_FindCertByType(MQTT_CERT_TYPE_CA_ROOT, alias, sizeof(alias)));
    TEST_ASSERT_EQUAL_STRING("ca-root", alias);
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK,
        Mqtt_CertMgr_FindCertByType(MQTT_CERT_TYPE_SERVER, alias, sizeof(alias)));
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_CertMgr_FindCertByType(MQTT_CERT_TYPE_CA_ROOT, NULL_PTR, sizeof(alias)));
}

/*============================================================================
 * MainFunction / callbacks / import-export
 *============================================================================*/

void test_CertMgr_MainFunction_WithoutInit_ShouldDoNothing(void) {
    rec_expiry_calls = 0;
    Mqtt_CertMgr_DeInit();
    Mqtt_CertMgr_MainFunction();
    TEST_ASSERT_EQUAL_INT(0, rec_expiry_calls);
}

void test_CertMgr_MainFunction_WithinInterval_ShouldSkipScan(void) {
    Mqtt_CertMgrConfigType cfg;
    Mqtt_CertEntryType entry;
    defaultCfg(&cfg);
    cfg.checkIntervalMs = 60000U; /* currentTime is frozen -> diff(0) < 60 */
    Mqtt_CertMgr_DeInit();
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_CertMgr_Init(&cfg));
    stub_cert_valid_from_year = 0;
    stub_cert_valid_to_year = 0;
    entry = makeEntry("soon", MQTT_CERT_TYPE_SERVER);
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_CertMgr_AddCert(&entry));
    rec_expiry_calls = 0;
    Mqtt_CertMgr_MainFunction();
    TEST_ASSERT_EQUAL_INT(0, rec_expiry_calls);
}

void test_CertMgr_MainFunction_ScanDue_ShouldFireExpiryCallback(void) {
    Mqtt_CertMgrConfigType cfg;
    Mqtt_CertEntryType entry;
    /* setUp uses the NULL-config default (checkIntervalMs=60000); with the
     * frozen clock the scan would never run. Re-init with interval 0. */
    defaultCfg(&cfg); /* checkIntervalMs = 0 */
    Mqtt_CertMgr_DeInit();
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_CertMgr_Init(&cfg));
    stub_cert_valid_from_year = 0;
    stub_cert_valid_to_year = 0; /* already "expired" */
    entry = makeEntry("soon", MQTT_CERT_TYPE_SERVER);
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_CertMgr_AddCert(&entry));
    rec_expiry_calls = 0;
    Mqtt_CertMgr_SetExpiryCallback(&recordExpiryCb);
    Mqtt_CertMgr_MainFunction();
    TEST_ASSERT_EQUAL_INT(1, rec_expiry_calls);
    TEST_ASSERT_EQUAL_STRING("soon", rec_expiry_alias);
}

void test_CertMgr_ImportFromPem_ShouldAddCertificate(void) {
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK,
        Mqtt_CertMgr_ImportFromPem(fakePem, "imported", MQTT_CERT_TYPE_CA_ROOT));
    TEST_ASSERT_EQUAL_UINT8(1U, Mqtt_CertMgr_GetCertCount());
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_CertMgr_ImportFromPem(NULL_PTR, "x", MQTT_CERT_TYPE_CA_ROOT));
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_CertMgr_ImportFromPem(fakePem, NULL_PTR, MQTT_CERT_TYPE_CA_ROOT));
}

void test_CertMgr_ExportToPem_ShouldReturnStoredBytes(void) {
    char out[64];
    uint32 written = 0U;
    addCaRoot();
    memset(out, 0, sizeof(out));
    TEST_ASSERT_EQUAL_UINT8(MQTT_OK, Mqtt_CertMgr_ExportToPem("ca-root", out, sizeof(out), &written));
    TEST_ASSERT_EQUAL_UINT32((uint32)strlen(fakePem) + 1U, written);
    TEST_ASSERT_EQUAL_STRING(fakePem, out);
}

void test_CertMgr_ExportToPem_ParameterChecks_ShouldFail(void) {
    char out[64];
    uint32 written = 0U;
    addCaRoot();
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_CertMgr_ExportToPem(NULL_PTR, out, sizeof(out), &written));
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_CertMgr_ExportToPem("ca-root", NULL_PTR, sizeof(out), &written));
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_CertMgr_ExportToPem("ca-root", out, 0U, &written));
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_CertMgr_ExportToPem("ca-root", out, sizeof(out), NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_CertMgr_ExportToPem("missing", out, sizeof(out), &written));
    /* Buffer too small for the stored data. */
    TEST_ASSERT_EQUAL_UINT8(MQTT_E_NOT_OK, Mqtt_CertMgr_ExportToPem("ca-root", out, 4U, &written));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_CertMgr_Init_NullConfig_ShouldUseDefaults);
    RUN_TEST(test_CertMgr_Init_CustomConfig_ShouldBeIdempotent);
    RUN_TEST(test_CertMgr_DeInit_ShouldClearStore);

    RUN_TEST(test_CertMgr_AddCert_WithoutInit_ShouldFail);
    RUN_TEST(test_CertMgr_AddCert_NullEntry_ShouldFail);
    RUN_TEST(test_CertMgr_AddCert_TooLarge_ShouldFail);
    RUN_TEST(test_CertMgr_AddCert_StoreFull_ShouldFail);
    RUN_TEST(test_CertMgr_AddCert_DuplicateAlias_ShouldUpdateInPlace);
    RUN_TEST(test_CertMgr_AddCert_ShouldParseSubjectAndExtensions);
    RUN_TEST(test_CertMgr_AddCert_UnparseableData_ShouldUseDefaults);
    RUN_TEST(test_CertMgr_RemoveCert_ShouldFreeSlot);
    RUN_TEST(test_CertMgr_RemoveCert_ParameterChecks_ShouldFail);
    RUN_TEST(test_CertMgr_GetCert_ParameterChecks_ShouldFail);
    RUN_TEST(test_CertMgr_UpdateCert_ShouldReplaceDataAndNotify);
    RUN_TEST(test_CertMgr_UpdateCert_ParameterChecks_ShouldFail);

    RUN_TEST(test_CertMgr_LoadCertForTls_ShouldExposePooledData);
    RUN_TEST(test_CertMgr_LoadCertForTls_ParameterChecks_ShouldFail);
    RUN_TEST(test_CertMgr_LoadKeyForTls_ShouldExposeKeyData);
    RUN_TEST(test_CertMgr_LoadKeyForTls_MissingAlias_ShouldFail);
    RUN_TEST(test_CertMgr_LoadTrustStore_ByNameAndWildcard);
    RUN_TEST(test_CertMgr_LoadTrustStore_NoCaAvailable_ShouldFail);

    RUN_TEST(test_CertMgr_ValidateCert_ParameterChecks_ShouldFail);
    RUN_TEST(test_CertMgr_ValidateCert_ParseFailure_ShouldReportInvalidFormat);
    RUN_TEST(test_CertMgr_ValidateCert_WithoutInit_ShouldFail);
    RUN_TEST(test_CertMgr_ValidateCert_UnknownAlias_ShouldFail);
    RUN_TEST(test_CertMgr_ValidateCert_NotYetValidYears_ShouldReportNotYetValid);
    RUN_TEST(test_CertMgr_ValidateCert_ExpiredYears_ShouldReportExpired);
    RUN_TEST(test_CertMgr_ValidateCert_WindowYears_ShouldReachVerifyBranches);
    RUN_TEST(test_CertMgr_ValidateCertChain_WithCa_ShouldVerify);
    RUN_TEST(test_CertMgr_ValidateCertChain_ParseFailure_ShouldReportInvalid);
    RUN_TEST(test_CertMgr_ValidateCertChain_ParameterChecks_ShouldFail);
    RUN_TEST(test_CertMgr_CheckExpiry_ShouldDetectExpiringCertificates);
    RUN_TEST(test_CertMgr_CheckExpiry_ParameterChecks_ShouldFail);

    RUN_TEST(test_CertMgr_GetCertCount_WithoutInit_ShouldBeZero);
    RUN_TEST(test_CertMgr_GetCertAliasByIndex_ShouldEnumerate);
    RUN_TEST(test_CertMgr_FindCertByType_ShouldFindAndReportMissing);

    RUN_TEST(test_CertMgr_MainFunction_WithoutInit_ShouldDoNothing);
    RUN_TEST(test_CertMgr_MainFunction_WithinInterval_ShouldSkipScan);
    RUN_TEST(test_CertMgr_MainFunction_ScanDue_ShouldFireExpiryCallback);
    RUN_TEST(test_CertMgr_ImportFromPem_ShouldAddCertificate);
    RUN_TEST(test_CertMgr_ExportToPem_ShouldReturnStoredBytes);
    RUN_TEST(test_CertMgr_ExportToPem_ParameterChecks_ShouldFail);
    return UnityEnd();
}
