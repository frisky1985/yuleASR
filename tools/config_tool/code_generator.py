"""
Code Generator Module for DDS Configuration Tool.

Generates C header files from diagnostic, E2E, and OTA configurations.
Supports JSON Schema validation and auto-generation of configuration structures.

Author: Hermes Agent
Version: 1.0.0
"""

import json
from pathlib import Path
from typing import Dict, List, Any, Optional, Union
from datetime import datetime
from dataclasses import asdict

try:
    import jsonschema
    from jsonschema import validate, ValidationError
    HAS_JSONSCHEMA = True
except ImportError:
    HAS_JSONSCHEMA = False

from diagnostic_config import DiagnosticConfig, DiagServiceConfig, DIDDefinition, DTCConfig
from diagnostic_config import SecurityPolicy, DCMParams, DoIPConfig, DiagSessionType
from e2e_config import E2EConfig, E2EProfileConfig, DataIDMapping, E2EProtection, E2EMonitoring
from e2e_config import E2EProfile, E2EDataIDMode, CRCType
from ota_config import OTAConfig, PartitionConfig, ABPartitionLayout, OTACampaign
from ota_config import DownloadParams, SecurityVerification, ECUUpdate, OTAState


class CCodeGenerator:
    """
    C header file generator for automotive configurations.
    
    Generates type-safe C structures and constants from configuration objects.
    """
    
    def __init__(self, header_guard_prefix: str = "GENERATED_CONFIG"):
        self.header_guard_prefix = header_guard_prefix
        self.indent = "    "
        
    def _generate_header_guard(self, filename: str) -> str:
        """Generate header guard macro name."""
        guard = f"{self.header_guard_prefix}_{filename.upper().replace('.', '_')}"
        return guard
    
    def _generate_file_header(self, filename: str, description: str) -> str:
        """Generate file header comment."""
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        return f"""/**
 * @file {filename}
 * @brief {description}
 * 
 * @note AUTO-GENERATED FILE - DO NOT MODIFY
 * @generated {timestamp}
 * @generator DDS Configuration Tool
 * @version 1.0.0
 */

#ifndef {self._generate_header_guard(filename)}
#define {self._generate_header_guard(filename)}

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

"""

    def _generate_file_footer(self, filename: str) -> str:
        """Generate file footer with header guard end."""
        return f"""
#endif /* {self._generate_header_guard(filename)} */
"""

    def _indent(self, level: int) -> str:
        """Get indentation string."""
        return self.indent * level

    def generate_diagnostic_header(self, config: DiagnosticConfig, filename: str = "diagnostic_config.h") -> str:
        """
        Generate C header for diagnostic configuration.
        
        Args:
            config: Diagnostic configuration
            filename: Output filename for header guard
            
        Returns:
            Generated C header content
        """
        code = self._generate_file_header(filename, "Diagnostic Configuration")
        
        # Generate includes
        code += "/* Standard AUTOSAR Diagnostic Types */\n"
        code += "#include \"Std_Types.h\"\n"
        code += "#include \"Dcm_Types.h\"\n\n"
        
        # Generate DCM Parameters
        if config.dcm_params:
            code += self._generate_dcm_params(config.dcm_params)
            code += "\n"
        
        # Generate Session Types
        code += self._generate_session_types(config.supported_sessions)
        code += "\n"
        
        # Generate Service IDs
        if config.services:
            code += self._generate_service_ids(config.services)
            code += "\n"
        
        # Generate DID Definitions
        if config.dids:
            code += self._generate_did_definitions(config.dids)
            code += "\n"
        
        # Generate DTC Configurations
        if config.dtcs:
            code += self._generate_dtc_config(config.dtcs)
            code += "\n"
        
        # Generate Security Policies
        if config.security_policies:
            code += self._generate_security_policies(config.security_policies)
            code += "\n"
        
        # Generate DoIP Configuration
        if config.doip_config.enabled:
            code += self._generate_doip_config(config.doip_config)
            code += "\n"
        
        code += self._generate_file_footer(filename)
        return code

    def _generate_dcm_params(self, params: DCMParams) -> str:
        """Generate DCM parameter defines."""
        code = "/* DCM (Diagnostic Communication Manager) Parameters */\n"
        code += f"#define DCM_ECU_ID                      (0x{params.ecu_id:04X}U)\n"
        code += f"#define DCM_LOGICAL_ADDRESS             (0x{params.logical_address:04X}U)\n"
        code += f"#define DCM_FUNCTIONAL_ADDRESS          (0x{params.functional_address:04X}U)\n"
        code += f"#define DCM_MAX_RESPONSE_LENGTH         ({params.max_response_length}U)\n"
        code += f"#define DCM_P2_TIMEOUT_MS               ({params.p2_timeout_ms}U)\n"
        code += f"#define DCM_P2_STAR_TIMEOUT_MS          ({params.p2_star_timeout_ms}U)\n"
        code += f"#define DCM_S3_TIMEOUT_MS               ({params.s3_timeout_ms}U)\n"
        code += f"#define DCM_DEFAULT_SESSION_TIMEOUT_MS  ({params.default_session_timeout_ms}U)\n"
        code += f"#define DCM_TESTER_PRESENT_REQUIRED     ({'TRUE' if params.tester_present_required else 'FALSE'})\n"
        code += f"#define DCM_TESTER_PRESENT_PERIOD_MS    ({params.tester_present_period_ms}U)\n"
        return code

    def _generate_session_types(self, sessions: List[DiagSessionType]) -> str:
        """Generate session type defines."""
        code = "/* Diagnostic Session Types */\n"
        session_map = {
            DiagSessionType.DEFAULT: ("DCM_DEFAULT_SESSION", 0x01),
            DiagSessionType.PROGRAMMING: ("DCM_PROGRAMMING_SESSION", 0x02),
            DiagSessionType.EXTENDED: ("DCM_EXTENDED_DIAGNOSTIC_SESSION", 0x03),
            DiagSessionType.SAFETY_SYSTEM: ("DCM_SAFETY_SYSTEM_DIAGNOSTIC_SESSION", 0x04)
        }
        
        for session in sessions:
            if session in session_map:
                name, value = session_map[session]
                code += f"#define {name:40s} (0x{value:02X}U)\n"
        
        return code

    def _generate_service_ids(self, services: List[DiagServiceConfig]) -> str:
        """Generate service ID defines and configuration table."""
        code = "/* Diagnostic Service IDs */\n"
        
        for svc in services:
            code += f"#define DCM_SVC_{svc.name.upper():30s} (0x{svc.service_id:02X}U)\n"
        
        code += "\n/* Service Configuration */\n"
        code += "typedef struct {\n"
        code += f"{self._indent(1)}uint8_t serviceId;\n"
        code += f"{self._indent(1)}uint8_t priority;\n"
        code += f"{self._indent(1)}boolean enabled;\n"
        code += f"{self._indent(1)}uint8_t supportedSessions;\n"
        code += f"{self._indent(1)}uint16_t timeoutMs;\n"
        code += f"{self._indent(1)}uint8_t securityLevel;\n"
        code += "} Dcm_ServiceConfigType;\n\n"
        
        code += f"#define DCM_NUM_SERVICES                ({len(services)}U)\n\n"
        code += "extern const Dcm_ServiceConfigType Dcm_ServiceConfig[DCM_NUM_SERVICES];\n"
        
        return code

    def _generate_did_definitions(self, dids: List[DIDDefinition]) -> str:
        """Generate DID definitions."""
        code = "/* Data Identifier (DID) Definitions */\n"
        
        for did in dids:
            code += f"#define DCM_DID_{did.name.upper():30s} (0x{did.did:04X}U)  /* {did.description} */\n"
        
        code += f"\n#define DCM_NUM_DIDS                    ({len(dids)}U)\n\n"
        
        code += "typedef struct {\n"
        code += f"{self._indent(1)}uint16_t did;\n"
        code += f"{self._indent(1)}uint16_t dataLength;\n"
        code += f"{self._indent(1)}uint8_t readSecurityLevel;\n"
        code += f"{self._indent(1)}uint8_t writeSecurityLevel;\n"
        code += f"{self._indent(1)}const void* readCallback;\n"
        code += f"{self._indent(1)}const void* writeCallback;\n"
        code += "} Dcm_DidConfigType;\n\n"
        
        code += "extern const Dcm_DidConfigType Dcm_DidConfig[DCM_NUM_DIDS];\n"
        
        return code

    def _generate_dtc_config(self, dtcs: List[DTCConfig]) -> str:
        """Generate DTC configuration."""
        code = "/* Diagnostic Trouble Code (DTC) Configuration */\n"
        code += f"#define DEM_NUM_DTC                     ({len(dtcs)}U)\n\n"
        
        code += "typedef struct {\n"
        code += f"{self._indent(1)}uint32_t dtc;\n"
        code += f"{self._indent(1)}uint8_t severity;\n"
        code += f"{self._indent(1)}uint8_t debounceThreshold;\n"
        code += f"{self._indent(1)}uint8_t agingThreshold;\n"
        code += f"{self._indent(1)}boolean freezeFrameEnabled;\n"
        code += f"{self._indent(1)}uint8_t freezeFrameRecords;\n"
        code += "} Dem_DtcConfigType;\n\n"
        
        code += "extern const Dem_DtcConfigType Dem_DtcConfig[DEM_NUM_DTC];\n"
        
        return code

    def _generate_security_policies(self, policies: List[SecurityPolicy]) -> str:
        """Generate security policy defines."""
        code = "/* Security Access Policies */\n"
        code += f"#define DCM_NUM_SECURITY_LEVELS         ({len(policies)}U)\n\n"
        
        for policy in policies:
            code += f"#define DCM_SEC_{policy.name.upper():30s} ({policy.level}U)\n"
        
        code += "\ntypedef struct {\n"
        code += f"{self._indent(1)}uint8_t level;\n"
        code += f"{self._indent(1)}uint8_t seedLength;\n"
        code += f"{self._indent(1)}uint8_t keyLength;\n"
        code += f"{self._indent(1)}uint8_t maxAttempts;\n"
        code += f"{self._indent(1)}uint16_t delayTimerMs;\n"
        code += "} Dcm_SecurityConfigType;\n\n"
        
        code += "extern const Dcm_SecurityConfigType Dcm_SecurityConfig[DCM_NUM_SECURITY_LEVELS];\n"
        
        return code

    def _generate_doip_config(self, doip: DoIPConfig) -> str:
        """Generate DoIP configuration."""
        code = "/* DoIP (Diagnostic over IP) Configuration */\n"
        code += f"#define DOIP_ENABLED                    ({'TRUE' if doip.enabled else 'FALSE'})\n"
        code += f"#define DOIP_LOCAL_PORT                 ({doip.local_port}U)\n"
        code += f"#define DOIP_DISCOVERY_PORT             ({doip.discovery_port}U)\n"
        code += f"#define DOIP_MAX_CONNECTIONS            ({doip.max_connections}U)\n"
        code += f"#define DOIP_ROUTING_ACTIVATION_TIMEOUT ({doip.routing_activation_timeout_ms}U)\n"
        code += f"#define DOIP_ALIVE_CHECK_INTERVAL       ({doip.alive_check_interval_ms}U)\n"
        code += f"#define DOIP_VEHICLE_ANNOUNCEMENT_INTERVAL ({doip.vehicle_announcement_interval_ms}U)\n"
        code += f"#define DOIP_VEHICLE_ANNOUNCEMENT_COUNT ({doip.vehicle_announcement_count}U)\n"
        return code

    def generate_e2e_header(self, config: E2EConfig, filename: str = "e2e_config.h") -> str:
        """
        Generate C header for E2E configuration.
        
        Args:
            config: E2E configuration
            filename: Output filename for header guard
            
        Returns:
            Generated C header content
        """
        code = self._generate_file_header(filename, "E2E (End-to-End) Protection Configuration")
        
        code += "/* AUTOSAR E2E Types */\n"
        code += "#include \"E2E_Types.h\"\n"
        code += "#include \"Csm_Types.h\"\n\n"
        
        # Generate E2E Profiles
        if config.profiles:
            code += self._generate_e2e_profiles(config.profiles)
            code += "\n"
        
        # Generate Data ID Mappings
        if config.data_id_mappings:
            code += self._generate_data_id_mappings(config.data_id_mappings)
            code += "\n"
        
        # Generate Protection Configurations
        if config.protections:
            code += self._generate_e2e_protections(config.protections)
            code += "\n"
        
        # Generate Monitoring Configurations
        if config.monitorings:
            code += self._generate_e2e_monitorings(config.monitorings)
            code += "\n"
        
        # Generate CRC Polynomials
        if config.crc_polynomials:
            code += self._generate_crc_polynomials(config.crc_polynomials)
            code += "\n"
        
        code += self._generate_file_footer(filename)
        return code

    def _generate_e2e_profiles(self, profiles: Dict[E2EProfile, E2EProfileConfig]) -> str:
        """Generate E2E profile configurations."""
        code = "/* E2E Profile Configurations */\n"
        code += f"#define E2E_NUM_PROFILES                ({len(profiles)}U)\n\n"
        
        for profile, cfg in profiles.items():
            code += f"/* Profile {profile.value} */\n"
            code += f"#define E2E_P{profile.value}_CRC_OFFSET         ({cfg.crc_offset}U)\n"
            code += f"#define E2E_P{profile.value}_COUNTER_OFFSET     ({cfg.counter_offset}U)\n"
            code += f"#define E2E_P{profile.value}_DATA_LENGTH        ({cfg.data_length}U)\n"
            code += f"#define E2E_P{profile.value}_MAX_DELTA_COUNTER  ({cfg.max_delta_counter_init}U)\n"
            code += "\n"
        
        code += "typedef struct {\n"
        code += f"{self._indent(1)}uint8_t profile;\n"
        code += f"{self._indent(1)}uint8_t crcOffset;\n"
        code += f"{self._indent(1)}uint8_t counterOffset;\n"
        code += f"{self._indent(1)}uint16_t dataId;\n"
        code += f"{self._indent(1)}uint16_t dataLength;\n"
        code += f"{self._indent(1)}uint8_t maxDeltaCounter;\n"
        code += "} E2E_ProfileConfigType;\n\n"
        
        return code

    def _generate_data_id_mappings(self, mappings: List[DataIDMapping]) -> str:
        """Generate Data ID mappings."""
        code = "/* E2E Data ID Mappings */\n"
        
        for mapping in mappings:
            code += f"#define E2E_DATAID_{mapping.name.upper():30s} (0x{mapping.data_id:04X}U)\n"
        
        code += f"\n#define E2E_NUM_DATAID_MAPPINGS         ({len(mappings)}U)\n\n"
        
        code += "typedef struct {\n"
        code += f"{self._indent(1)}uint16_t dataId;\n"
        code += f"{self._indent(1)}uint8_t profile;\n"
        code += f"{self._indent(1)}uint8_t crcType;\n"
        code += f"{self._indent(1)}const char* sourceComponent;\n"
        code += f"{self._indent(1)}const char* targetComponent;\n"
        code += "} E2E_DataIdMappingType;\n\n"
        
        code += "extern const E2E_DataIdMappingType E2E_DataIdMapping[E2E_NUM_DATAID_MAPPINGS];\n"
        
        return code

    def _generate_e2e_protections(self, protections: List[E2EProtection]) -> str:
        """Generate E2E protection configurations."""
        code = "/* E2E Protection Configurations */\n"
        code += f"#define E2E_NUM_PROTECTIONS             ({len(protections)}U)\n\n"
        
        code += "typedef struct {\n"
        code += f"{self._indent(1)}const char* name;\n"
        code += f"{self._indent(1)}uint16_t dataId;\n"
        code += f"{self._indent(1)}uint8_t headerLength;\n"
        code += f"{self._indent(1)}uint8_t windowSize;\n"
        code += f"{self._indent(1)}uint16_t timeoutMs;\n"
        code += "} E2E_ProtectionConfigType;\n\n"
        
        code += "extern const E2E_ProtectionConfigType E2E_ProtectionConfig[E2E_NUM_PROTECTIONS];\n"
        
        return code

    def _generate_e2e_monitorings(self, monitorings: List[E2EMonitoring]) -> str:
        """Generate E2E monitoring configurations."""
        code = "/* E2E Monitoring Configurations */\n"
        code += f"#define E2E_NUM_MONITORINGS             ({len(monitorings)}U)\n\n"
        
        code += "typedef struct {\n"
        code += f"{self._indent(1)}const char* name;\n"
        code += f"{self._indent(1)}uint16_t dataId;\n"
        code += f"{self._indent(1)}uint8_t maxDeltaCounter;\n"
        code += f"{self._indent(1)}uint8_t maxErrorStateInit;\n"
        code += f"{self._indent(1)}uint8_t maxErrorStateInvalid;\n"
        code += f"{self._indent(1)}uint8_t windowSize;\n"
        code += "} E2E_MonitoringConfigType;\n\n"
        
        code += "extern const E2E_MonitoringConfigType E2E_MonitoringConfig[E2E_NUM_MONITORINGS];\n"
        
        return code

    def _generate_crc_polynomials(self, polynomials: Dict[CRCType, Any]) -> str:
        """Generate CRC polynomial definitions."""
        code = "/* CRC Polynomial Definitions */\n"
        
        for crc_type, poly in polynomials.items():
            name = crc_type.value.upper()
            code += f"#define CRC_{name}_POLYNOMIAL           (0x{poly.polynomial:08X}U)\n"
            code += f"#define CRC_{name}_INITIAL              (0x{poly.initial_value:08X}U)\n"
            code += f"#define CRC_{name}_XOR                  (0x{poly.final_xor:08X}U)\n"
            code += "\n"
        
        return code

    def generate_ota_header(self, config: OTAConfig, filename: str = "ota_config.h") -> str:
        """
        Generate C header for OTA configuration.
        
        Args:
            config: OTA configuration
            filename: Output filename for header guard
            
        Returns:
            Generated C header content
        """
        code = self._generate_file_header(filename, "OTA (Over-The-Air) Update Configuration")
        
        code += "/* AUTOSAR OTA Types */\n"
        code += "#include \"Ota_Types.h\"\n"
        code += "#include \"Fota_Types.h\"\n\n"
        
        # Generate OTA States
        code += self._generate_ota_states()
        code += "\n"
        
        # Generate Partition Configurations
        if config.partition_layouts:
            code += self._generate_partition_configs(config.partition_layouts)
            code += "\n"
        
        # Generate Download Parameters
        code += self._generate_download_params(config.download_params)
        code += "\n"
        
        # Generate Security Verification
        code += self._generate_ota_security(config.security_verification)
        code += "\n"
        
        # Generate Campaign Configurations
        if config.campaigns:
            code += self._generate_campaign_configs(config.campaigns)
            code += "\n"
        
        code += self._generate_file_footer(filename)
        return code

    def _generate_ota_states(self) -> str:
        """Generate OTA state definitions."""
        code = "/* OTA State Machine States */\n"
        states = [
            ("OTA_STATE_IDLE", 0x00),
            ("OTA_STATE_CAMPAIGN_RECEIVED", 0x01),
            ("OTA_STATE_DOWNLOADING", 0x02),
            ("OTA_STATE_VERIFYING", 0x03),
            ("OTA_STATE_READY_TO_INSTALL", 0x04),
            ("OTA_STATE_INSTALLING", 0x05),
            ("OTA_STATE_ACTIVATING", 0x06),
            ("OTA_STATE_VERIFYING_BOOT", 0x07),
            ("OTA_STATE_SUCCESS", 0x08),
            ("OTA_STATE_ROLLING_BACK", 0x10),
            ("OTA_STATE_FAILED", 0xFF)
        ]
        
        for name, value in states:
            code += f"#define {name:30s} (0x{value:02X}U)\n"
        
        return code

    def _generate_partition_configs(self, layouts: List[ABPartitionLayout]) -> str:
        """Generate partition configurations."""
        code = "/* Flash Partition Configuration */\n"
        code += f"#define OTA_NUM_PARTITION_LAYOUTS       ({len(layouts)}U)\n\n"
        
        for layout in layouts:
            code += f"/* Layout: {layout.name} */\n"
            all_parts = layout.bank_a_partitions + layout.bank_b_partitions + layout.shared_partitions
            code += f"#define OTA_{layout.name.upper()}_NUM_PARTITIONS ({len(all_parts)}U)\n"
            
            for part in all_parts:
                prefix = f"OTA_PART_{layout.name.upper()}_{part.name.upper()}"
                code += f"#define {prefix}_START        (0x{part.start_address:08X}U)\n"
                code += f"#define {prefix}_SIZE         (0x{part.size_bytes:08X}U)\n"
                code += f"#define {prefix}_TYPE         ({part.partition_type.value.upper()})\n"
            code += "\n"
        
        code += "typedef struct {\n"
        code += f"{self._indent(1)}const char* name;\n"
        code += f"{self._indent(1)}uint32_t startAddress;\n"
        code += f"{self._indent(1)}uint32_t size;\n"
        code += f"{self._indent(1)}uint8_t type;\n"
        code += f"{self._indent(1)}boolean isActive;\n"
        code += f"{self._indent(1)}boolean isBootable;\n"
        code += "} Ota_PartitionConfigType;\n\n"
        
        return code

    def _generate_download_params(self, params: DownloadParams) -> str:
        """Generate download parameter defines."""
        code = "/* OTA Download Parameters */\n"
        code += f"#define OTA_CHUNK_SIZE                  ({params.chunk_size_bytes}U)\n"
        code += f"#define OTA_MAX_RETRIES                 ({params.max_retries}U)\n"
        code += f"#define OTA_RETRY_DELAY_MS              ({params.retry_delay_ms}U)\n"
        code += f"#define OTA_DOWNLOAD_TIMEOUT_MS         ({params.timeout_ms}U)\n"
        code += f"#define OTA_CONCURRENT_DOWNLOADS        ({params.concurrent_downloads}U)\n"
        code += f"#define OTA_RESUME_SUPPORTED            ({'TRUE' if params.resume_supported else 'FALSE'})\n"
        code += f"#define OTA_COMPRESSION_TYPE            \"{params.compression}\"\n"
        code += f"#define OTA_ENCRYPTION_TYPE             \"{params.encryption}\"\n"
        code += f"#define OTA_DELTA_UPDATE                ({'TRUE' if params.delta_update else 'FALSE'})\n"
        code += f"#define OTA_DELTA_ALGORITHM             \"{params.delta_algorithm}\"\n"
        
        return code

    def _generate_ota_security(self, security: SecurityVerification) -> str:
        """Generate OTA security configuration."""
        code = "/* OTA Security Verification */\n"
        code += f"#define OTA_SIGNATURE_REQUIRED          ({'TRUE' if security.signature_required else 'FALSE'})\n"
        code += f"#define OTA_SIGNATURE_ALGORITHM         \"{security.signature_algorithm}\"\n"
        code += f"#define OTA_HASH_ALGORITHM              \"{security.hash_algorithm}\"\n"
        code += f"#define OTA_VERIFY_CERT_CHAIN           ({'TRUE' if security.verify_certificate_chain else 'FALSE'})\n"
        code += f"#define OTA_VERIFY_ROLLBACK             ({'TRUE' if security.verify_version_rollback else 'FALSE'})\n"
        code += f"#define OTA_SECURE_BOOT_REQUIRED        ({'TRUE' if security.secure_boot_required else 'FALSE'})\n"
        
        if security.root_cert_hash:
            code += f"#define OTA_ROOT_CERT_HASH              \"{security.root_cert_hash}\"\n"
        
        return code

    def _generate_campaign_configs(self, campaigns: List[OTACampaign]) -> str:
        """Generate campaign configurations."""
        code = "/* OTA Campaign Configurations */\n"
        code += f"#define OTA_NUM_CAMPAIGNS               ({len(campaigns)}U)\n\n"
        
        for campaign in campaigns:
            code += f"/* Campaign: {campaign.campaign_id} */\n"
            code += f"#define OTA_CAMPAIGN_{campaign.campaign_id.upper()}_NUM_UPDATES ({len(campaign.ecu_updates)}U)\n"
            
            for update in campaign.ecu_updates:
                prefix = f"OTA_UPDATE_{campaign.campaign_id.upper()}_{update.name.upper()}"
                code += f"#define {prefix}_ECU_ID       (0x{update.ecu_id:04X}U)\n"
                code += f"#define {prefix}_VERSION_FROM \"{update.firmware_version_from}\"\n"
                code += f"#define {prefix}_VERSION_TO   \"{update.firmware_version_to}\"\n"
                code += f"#define {prefix}_SIZE         ({update.package_size}UL)\n"
            code += "\n"
        
        code += "typedef struct {\n"
        code += f"{self._indent(1)}const char* campaignId;\n"
        code += f"{self._indent(1)}const char* name;\n"
        code += f"{self._indent(1)}uint8_t priority;\n"
        code += f"{self._indent(1)}uint16_t numEcuUpdates;\n"
        code += "} Ota_CampaignConfigType;\n\n"
        
        return code

    def generate_all(self, 
                     diag_config: Optional[DiagnosticConfig] = None,
                     e2e_config: Optional[E2EConfig] = None,
                     ota_config: Optional[OTAConfig] = None,
                     output_dir: Union[str, Path] = ".") -> Dict[str, str]:
        """
        Generate all configuration headers.
        
        Args:
            diag_config: Diagnostic configuration (optional)
            e2e_config: E2E configuration (optional)
            ota_config: OTA configuration (optional)
            output_dir: Output directory for generated files
            
        Returns:
            Dictionary of filename -> generated code
        """
        results = {}
        
        if diag_config:
            results["diagnostic_config.h"] = self.generate_diagnostic_header(diag_config)
        
        if e2e_config:
            results["e2e_config.h"] = self.generate_e2e_header(e2e_config)
        
        if ota_config:
            results["ota_config.h"] = self.generate_ota_header(ota_config)
        
        return results

    def save_all(self, 
                 diag_config: Optional[DiagnosticConfig] = None,
                 e2e_config: Optional[E2EConfig] = None,
                 ota_config: Optional[OTAConfig] = None,
                 output_dir: Union[str, Path] = ".") -> List[str]:
        """
        Generate and save all configuration headers to files.
        
        Args:
            diag_config: Diagnostic configuration (optional)
            e2e_config: E2E configuration (optional)
            ota_config: OTA configuration (optional)
            output_dir: Output directory for generated files
            
        Returns:
            List of generated file paths
        """
        output_dir = Path(output_dir)
        output_dir.mkdir(parents=True, exist_ok=True)
        
        generated = []
        results = self.generate_all(diag_config, e2e_config, ota_config, output_dir)
        
        for filename, code in results.items():
            filepath = output_dir / filename
            with open(filepath, 'w', encoding='utf-8') as f:
                f.write(code)
            generated.append(str(filepath))
        
        return generated


class ConfigValidator:
    """
    Configuration validator with JSON Schema support.
    
    Validates diagnostic, E2E, and OTA configurations against schemas.
    """
    
    def __init__(self):
        self.schemas = self._generate_schemas()
    
    def _generate_schemas(self) -> Dict[str, Dict[str, Any]]:
        """Generate JSON schemas for configurations."""
        diagnostic_schema = {
            "$schema": "http://json-schema.org/draft-07/schema#",
            "type": "object",
            "properties": {
                "dcm_params": {
                    "type": "object",
                    "properties": {
                        "ecu_id": {"type": "string", "pattern": "^0x[0-9A-Fa-f]{4}$"},
                        "logical_address": {"type": "string", "pattern": "^0x[0-9A-Fa-f]{4}$"},
                        "functional_address": {"type": "string", "pattern": "^0x[0-9A-Fa-f]{4}$"},
                        "max_response_length": {"type": "integer", "minimum": 8, "maximum": 4095},
                        "p2_timeout_ms": {"type": "integer", "minimum": 1, "maximum": 10000},
                        "p2_star_timeout_ms": {"type": "integer", "minimum": 1, "maximum": 60000},
                        "s3_timeout_ms": {"type": "integer", "minimum": 1, "maximum": 10000},
                        "default_session_timeout_ms": {"type": "integer", "minimum": 1000, "maximum": 60000},
                        "tester_present_required": {"type": "boolean"},
                        "tester_present_period_ms": {"type": "integer", "minimum": 500, "maximum": 10000}
                    },
                    "required": ["ecu_id", "logical_address", "functional_address"]
                },
                "services": {
                    "type": "array",
                    "items": {
                        "type": "object",
                        "properties": {
                            "service_id": {"type": "string", "pattern": "^0x[0-9A-Fa-f]{2}$"},
                            "name": {"type": "string", "minLength": 1},
                            "priority": {"type": "string", "enum": ["P0", "P1", "P2", "P3"]},
                            "enabled": {"type": "boolean"},
                            "supported_sessions": {"type": "array", "items": {"type": "string"}},
                            "timeout_ms": {"type": "integer", "minimum": 100, "maximum": 60000},
                            "security_level_required": {"type": "integer", "minimum": 0, "maximum": 255}
                        },
                        "required": ["service_id", "name", "priority"]
                    }
                },
                "dids": {
                    "type": "array",
                    "items": {
                        "type": "object",
                        "properties": {
                            "did": {"type": "string", "pattern": "^0x[0-9A-Fa-f]{4}$"},
                            "name": {"type": "string", "minLength": 1},
                            "description": {"type": "string"},
                            "data_length": {"type": "integer", "minimum": 1, "maximum": 65535},
                            "data_type": {"type": "string"},
                            "read_security_level": {"type": "integer", "minimum": 0, "maximum": 255},
                            "write_security_level": {"type": "integer", "minimum": 0, "maximum": 255}
                        },
                        "required": ["did", "name", "data_length", "data_type"]
                    }
                },
                "dtcs": {
                    "type": "array",
                    "items": {
                        "type": "object",
                        "properties": {
                            "dtc": {"type": "string", "pattern": "^0x[0-9A-Fa-f]{6}$"},
                            "severity": {"type": "integer", "minimum": 0, "maximum": 7},
                            "fault_type": {"type": "string"},
                            "debounce_counter_threshold": {"type": "integer", "minimum": 1, "maximum": 255},
                            "aging_counter_threshold": {"type": "integer", "minimum": 1, "maximum": 255},
                            "enable_freeze_frame": {"type": "boolean"},
                            "freeze_frame_records": {"type": "integer", "minimum": 1, "maximum": 10},
                            "enable_extended_data": {"type": "boolean"}
                        },
                        "required": ["dtc", "severity", "fault_type"]
                    }
                },
                "security_policies": {
                    "type": "array",
                    "items": {
                        "type": "object",
                        "properties": {
                            "level": {"type": "integer", "minimum": 0, "maximum": 255},
                            "name": {"type": "string", "minLength": 1},
                            "seed_length": {"type": "integer", "minimum": 1, "maximum": 16},
                            "key_length": {"type": "integer", "minimum": 1, "maximum": 16},
                            "max_attempts": {"type": "integer", "minimum": 1, "maximum": 10},
                            "delay_timer_ms": {"type": "integer", "minimum": 1000, "maximum": 60000},
                            "algorithm": {"type": "string"}
                        },
                        "required": ["level", "name"]
                    }
                },
                "doip_config": {
                    "type": "object",
                    "properties": {
                        "enabled": {"type": "boolean"},
                        "local_port": {"type": "integer", "minimum": 1, "maximum": 65535},
                        "discovery_port": {"type": "integer", "minimum": 1, "maximum": 65535},
                        "max_connections": {"type": "integer", "minimum": 1, "maximum": 32},
                        "routing_activation_timeout_ms": {"type": "integer", "minimum": 1000, "maximum": 60000},
                        "alive_check_interval_ms": {"type": "integer", "minimum": 100, "maximum": 5000},
                        "vehicle_announcement_interval_ms": {"type": "integer", "minimum": 100, "maximum": 5000},
                        "vehicle_announcement_count": {"type": "integer", "minimum": 1, "maximum": 10},
                        "vin": {"type": "string"},
                        "eid": {"type": "string"},
                        "gid": {"type": "string"}
                    }
                }
            },
            "required": ["dcm_params", "services"]
        }
        
        return {
            "diagnostic": diagnostic_schema
        }
    
    def validate_diagnostic(self, config: DiagnosticConfig) -> List[str]:
        """
        Validate diagnostic configuration.
        
        Args:
            config: Diagnostic configuration to validate
            
        Returns:
            List of validation error messages
        """
        errors = config.validate()
        
        if HAS_JSONSCHEMA:
            try:
                data = config.to_dict()
                validate(instance=data, schema=self.schemas["diagnostic"])
            except ValidationError as e:
                errors.append(f"Schema validation: {e.message}")
        
        return errors
    
    def validate_e2e(self, config: E2EConfig) -> List[str]:
        """
        Validate E2E configuration.
        
        Args:
            config: E2E configuration to validate
            
        Returns:
            List of validation error messages
        """
        return config.validate()
    
    def validate_ota(self, config: OTAConfig) -> List[str]:
        """
        Validate OTA configuration.
        
        Args:
            config: OTA configuration to validate
            
        Returns:
            List of validation error messages
        """
        return config.validate()
    
    def validate_all(self,
                    diag_config: Optional[DiagnosticConfig] = None,
                    e2e_config: Optional[E2EConfig] = None,
                    ota_config: Optional[OTAConfig] = None) -> Dict[str, List[str]]:
        """
        Validate all configurations.
        
        Args:
            diag_config: Diagnostic configuration (optional)
            e2e_config: E2E configuration (optional)
            ota_config: OTA configuration (optional)
            
        Returns:
            Dictionary of config type -> list of errors
        """
        results = {}
        
        if diag_config:
            results["diagnostic"] = self.validate_diagnostic(diag_config)
        
        if e2e_config:
            results["e2e"] = self.validate_e2e(e2e_config)
        
        if ota_config:
            results["ota"] = self.validate_ota(ota_config)
        
        return results