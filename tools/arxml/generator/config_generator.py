#!/usr/bin/env python3
"""
AUTOSAR ARXML Configuration Generator
======================================
Generates C configuration code from ARXML parsed data.

Usage:
    python config_generator.py <parsed_json> --output-dir ./generated
    python config_generator.py --arxml <file.arxml> --output-dir ./generated

Supported Modules:
    - COM (Communication)
    - CanIf (CAN Interface)
    - NvM (NVRAM Manager)
    - PduR (PDU Router)
"""

import json
import sys
import os
from pathlib import Path
from typing import Dict, List, Any, Optional
from dataclasses import dataclass
from datetime import datetime


@dataclass
class GenerationContext:
    """Context for code generation."""
    module_name: str
    output_dir: Path
    timestamp: str
    version: str = "1.0.0"


class ConfigGenerator:
    """Main configuration generator class."""
    
    def __init__(self, output_dir: str = "./generated"):
        self.output_dir = Path(output_dir)
        self.output_dir.mkdir(parents=True, exist_ok=True)
        self.timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        
    def generate_from_json(self, json_file: str) -> Dict[str, Path]:
        """Generate configurations from parsed JSON."""
        with open(json_file, 'r', encoding='utf-8') as f:
            data = json.load(f)
        
        generated_files = {}
        
        # Generate COM configuration
        if 'com_config' in data or 'signals' in data:
            com_files = self._generate_com_config(data)
            generated_files.update(com_files)
        
        # Generate CanIf configuration
        if 'canif_config' in data or 'can_controllers' in data:
            canif_files = self._generate_canif_config(data)
            generated_files.update(canif_files)
        
        # Generate NvM configuration
        if 'nvm_config' in data or 'nv_blocks' in data:
            nvm_files = self._generate_nvm_config(data)
            generated_files.update(nvm_files)
        
        # Generate PduR configuration
        if 'pdur_config' in data or 'routing_paths' in data:
            pdur_files = self._generate_pdur_config(data)
            generated_files.update(pdur_files)
        
        return generated_files
    
    def _generate_header_guard(self, filename: str) -> str:
        """Generate header guard macro name."""
        base = Path(filename).stem.upper()
        return f"{base}_H"
    
    def _generate_file_header(self, filename: str, description: str) -> str:
        """Generate standard file header."""
        return f"""/*
 * {filename}
 * {description}
 *
 * Auto-generated from ARXML configuration
 * Generation Time: {self.timestamp}
 * Generator Version: 1.0.0
 *
 * DO NOT EDIT - This file is auto-generated
 */

"""

    def _generate_com_config(self, data: Dict) -> Dict[str, Path]:
        """Generate COM module configuration."""
        files = {}
        
        # Extract COM configuration from data
        com_config = data.get('com_config', {})
        signals = data.get('signals', [])
        ipdus = data.get('ipdus', [])
        signal_groups = data.get('signal_groups', [])
        
        # Generate Com_Cfg.h
        header_content = self._generate_com_cfg_h(com_config, signals, ipdus, signal_groups)
        header_path = self.output_dir / "Com_Cfg.h"
        header_path.write_text(header_content, encoding='utf-8')
        files['com_header'] = header_path
        
        # Generate Com_Cfg.c
        source_content = self._generate_com_cfg_c(com_config, signals, ipdus, signal_groups)
        source_path = self.output_dir / "Com_Cfg.c"
        source_path.write_text(source_content, encoding='utf-8')
        files['com_source'] = source_path
        
        return files
    
    def _generate_com_cfg_h(self, config: Dict, signals: List, ipdus: List, groups: List) -> str:
        """Generate Com_Cfg.h content."""
        guard = self._generate_header_guard("Com_Cfg.h")
        num_signals = len(signals) if signals else 8
        num_ipdus = len(ipdus) if ipdus else 4
        num_groups = len(groups) if groups else 2
        
        content = self._generate_file_header("Com_Cfg.h", "AUTOSAR COM Module Configuration Header")
        content += f"""#ifndef {guard}
#define {guard}

/*==================[Includes]==============================================*/
#include "Com_Types.h"
#include "ComStack_Types.h"

/*==================[Version Information]===================================*/
#define COM_CFG_SW_MAJOR_VERSION    1
#define COM_CFG_SW_MINOR_VERSION    0
#define COM_CFG_SW_PATCH_VERSION    0

/*==================[Pre-compile Configuration]=============================*/

/* Development Error Detection */
#define COM_DEV_ERROR_DETECT                STD_ON

/* Version Info API */
#define COM_VERSION_INFO_API                STD_ON

/* Enable Signal Group Array API */
#define COM_ENABLE_SIGNAL_GROUP_ARRAY_API   STD_ON

/*==================[Configuration Constants]===============================*/

/* Maximum number of elements */
#define COM_MAX_SIGNALS                     {num_signals}u
#define COM_MAX_SIGNAL_GROUPS               {num_groups}u
#define COM_MAX_IPDUS                       {num_ipdus}u
#define COM_MAX_IPDU_GROUPS                 4u

/* Maximum buffer sizes */
#define COM_MAX_IPDU_LENGTH                 64u
#define COM_MAX_SHADOW_BUFFER_SIZE          256u

/* Transmission Configuration */
#define COM_DEFAULT_TX_TIMEOUT              100u
#define COM_DEFAULT_MAX_RETRIES             3u
#define COM_RETRY_DELAY_MS                  10u

/*==================[Symbolic Names]========================================*/

/* IPdu IDs */
"""
        
        # Add IPdu symbolic names
        for i, ipdu in enumerate(ipdus if ipdus else []):
            name = ipdu.get('name', f'IPdu{i}')
            content += f"#define ComConf_ComIPdu_{name}          {i}u\n"
        
        if not ipdus:
            # Default IPdus
            content += """#define ComConf_ComIPdu_EngineData          0u
#define ComConf_ComIPdu_EngineStatus        1u
#define ComConf_ComIPdu_VehicleSpeed        2u
#define ComConf_ComIPdu_BodyControl         3u
"""
        
        content += "\n/* Signal IDs */\n"
        
        # Add Signal symbolic names
        for i, sig in enumerate(signals if signals else []):
            name = sig.get('name', f'Signal{i}')
            content += f"#define ComConf_ComSignal_{name}       {i}u\n"
        
        if not signals:
            # Default signals
            content += """#define ComConf_ComSignal_EngineSpeed       0u
#define ComConf_ComSignal_CoolantTemp       1u
#define ComConf_ComSignal_ThrottlePosition  2u
#define ComConf_ComSignal_VehicleSpeed      3u
"""
        
        content += f"""
/*==================[External Declarations]=================================*/

/* IPdu Configuration */
extern const Com_IPduType Com_IPduConfig[COM_MAX_IPDUS];

/* Signal Configuration */
extern const Com_SignalType Com_SignalConfig[COM_MAX_SIGNALS];

/* Signal Group Configuration */
extern const Com_SignalGroupType Com_SignalGroupConfig[COM_MAX_SIGNAL_GROUPS];

/* IPdu Group Configuration */
extern const Com_IPduGroupType Com_IPduGroupConfig[COM_MAX_IPDU_GROUPS];

/*==================[End of File]===========================================*/

#endif /* {guard} */
"""
        return content
    
    def _generate_com_cfg_c(self, config: Dict, signals: List, ipdus: List, groups: List) -> str:
        """Generate Com_Cfg.c content."""
        content = self._generate_file_header("Com_Cfg.c", "AUTOSAR COM Module Configuration Source")
        content += """/*==================[Includes]==============================================*/
#include "Com_Cfg.h"
#include "Com.h"

/*==================[Local Macros]==========================================*/

/*==================[Local Types]===========================================*/

/*==================[Local Data]============================================*/

/* IPdu Buffers */
"""
        
        # Generate IPdu buffers
        num_ipdus = len(ipdus) if ipdus else 4
        for i in range(num_ipdus):
            content += f"static uint8 Com_IPduBuffer_{i}[COM_MAX_IPDU_LENGTH];\n"
        
        content += "\n/*==================[Configuration Data]====================================*/\n\n"
        
        # Generate Signal Configuration
        content += "/* Signal Configuration */\n"
        content += "const Com_SignalType Com_SignalConfig[COM_MAX_SIGNALS] =\n{\n"
        
        num_signals = len(signals) if signals else 4
        for i in range(num_signals):
            sig = signals[i] if signals else {}
            name = sig.get('name', f'Signal{i}')
            length = sig.get('length', 8)
            start_bit = sig.get('start_bit', i * 8)
            
            content += f"    /* Signal {i}: {name} */\n"
            content += f"    {{\n"
            content += f"        /* handle */          {i}u,\n"
            content += f"        /* signalId */        ComConf_ComSignal_{name},\n"
            content += f"        /* ipduId */          0u,\n"
            content += f"        /* startBit */        {start_bit}u,\n"
            content += f"        /* bitLength */       {length}u,\n"
            content += f"        /* endianness */      COM_LITTLE_ENDIAN,\n"
            content += f"        /* signalType */      COM_UINT8,\n"
            content += f"        /* transferProperty */ COM_TRIGGERED,\n"
            content += f"        /* initValue */       0u\n"
            content += f"    }}{',' if i < num_signals - 1 else ''}\n"
        
        content += "};\n\n"
        
        # Generate IPdu Configuration
        content += "/* IPdu Configuration */\n"
        content += "const Com_IPduType Com_IPduConfig[COM_MAX_IPDUS] =\n{\n"
        
        for i in range(num_ipdus):
            ipdu = ipdus[i] if ipdus else {}
            name = ipdu.get('name', f'IPdu{i}')
            length = ipdu.get('length', 8)
            
            content += f"    /* IPdu {i}: {name} */\n"
            content += f"    {{\n"
            content += f"        /* handle */          {i}u,\n"
            content += f"        /* ipduId */          ComConf_ComIPdu_{name},\n"
            content += f"        /* length */          {length}u,\n"
            content += f"        /* signalProcessing */ COM_DEFERRED,\n"
            content += f"        /* ipduType */        COM_SEND,\n"
            content += f"        /* ipduDirection */   COM_SEND,\n"
            content += f"        /* ipduSignalRef */   NULL,\n"
            content += f"        /* bufferRef */       Com_IPduBuffer_{i}\n"
            content += f"    }}{',' if i < num_ipdus - 1 else ''}\n"
        
        content += "};\n\n"
        
        # Generate IPdu Group Configuration
        content += """/* IPdu Group Configuration */
const Com_IPduGroupType Com_IPduGroupConfig[COM_MAX_IPDU_GROUPS] =
{
    /* Group 0: Engine */
    {
        /* handle */        0u,
        /* ipduGroupId */   0u,
        /* ipduRefCount */  2u,
        /* ipduRef */       NULL
    },
    /* Group 1: Chassis */
    {
        /* handle */        1u,
        /* ipduGroupId */   1u,
        /* ipduRefCount */  1u,
        /* ipduRef */       NULL
    }
};

/*==================[End of File]===========================================*/
"""
        return content
    
    def _generate_canif_config(self, data: Dict) -> Dict[str, Path]:
        """Generate CanIf module configuration."""
        files = {}
        
        # Generate CanIf_Cfg.h
        header = self._generate_file_header("CanIf_Cfg.h", "AUTOSAR CAN Interface Configuration Header")
        header += """#ifndef CANIF_CFG_H
#define CANIF_CFG_H

/*==================[Includes]==============================================*/
#include "CanIf_Types.h"

/*==================[Version Information]===================================*/
#define CANIF_CFG_SW_MAJOR_VERSION    1
#define CANIF_CFG_SW_MINOR_VERSION    0
#define CANIF_CFG_SW_PATCH_VERSION    0

/*==================[Pre-compile Configuration]=============================*/
#define CANIF_DEV_ERROR_DETECT          STD_ON
#define CANIF_VERSION_INFO_API          STD_ON

/*==================[Configuration Constants]===============================*/
#define CANIF_MAX_CONTROLLERS           2u
#define CANIF_MAX_HTHS                  4u
#define CANIF_MAX_HRHS                  4u
#define CANIF_MAX_TX_PDUS               16u
#define CANIF_MAX_RX_PDUS               16u

/*==================[Symbolic Names]========================================*/
#define CanIfConf_CanIfCtrlCfg_CanCtrl0  0u
#define CanIfConf_CanIfCtrlCfg_CanCtrl1  1u

/*==================[External Declarations]=================================*/
extern const CanIf_ControllerCfgType CanIf_ControllerConfig[CANIF_MAX_CONTROLLERS];
extern const CanIf_HthCfgType CanIf_HthConfig[CANIF_MAX_HTHS];
extern const CanIf_HrhCfgType CanIf_HrhConfig[CANIF_MAX_HRHS];
extern const CanIf_TxPduCfgType CanIf_TxPduConfig[CANIF_MAX_TX_PDUS];
extern const CanIf_RxPduCfgType CanIf_RxPduConfig[CANIF_MAX_RX_PDUS];

#endif /* CANIF_CFG_H */
"""
        header_path = self.output_dir / "CanIf_Cfg.h"
        header_path.write_text(header, encoding='utf-8')
        files['canif_header'] = header_path
        
        return files
    
    def _generate_nvm_config(self, data: Dict) -> Dict[str, Path]:
        """Generate NvM module configuration."""
        files = {}
        
        header = self._generate_file_header("NvM_Cfg.h", "AUTOSAR NVRAM Manager Configuration Header")
        header += """#ifndef NVM_CFG_H
#define NVM_CFG_H

/*==================[Includes]==============================================*/
#include "NvM_Types.h"

/*==================[Version Information]===================================*/
#define NVM_CFG_SW_MAJOR_VERSION    1
#define NVM_CFG_SW_MINOR_VERSION    0
#define NVM_CFG_SW_PATCH_VERSION    0

/*==================[Pre-compile Configuration]=============================*/
#define NVM_DEV_ERROR_DETECT        STD_ON
#define NVM_VERSION_INFO_API        STD_ON
#define NVM_API_CONFIG_CLASS        NVM_API_CONFIG_CLASS_3

/*==================[Configuration Constants]===============================*/
#define NVM_MAX_NUM_OF_BLOCKS       32u
#define NVM_MAX_BLOCK_DATA_LENGTH   256u

/*==================[Symbolic Names]========================================*/
#define NvMConf_NvMBlockDescriptor_NvMBlock_Config    1u
#define NvMConf_NvMBlockDescriptor_NvMBlock_DTC       2u

/*==================[External Declarations]=================================*/
extern const NvM_BlockDescriptorType NvM_BlockDescriptorTable[NVM_MAX_NUM_OF_BLOCKS];

#endif /* NVM_CFG_H */
"""
        header_path = self.output_dir / "NvM_Cfg.h"
        header_path.write_text(header, encoding='utf-8')
        files['nvm_header'] = header_path
        
        return files
    
    def _generate_pdur_config(self, data: Dict) -> Dict[str, Path]:
        """Generate PduR module configuration."""
        files = {}
        
        header = self._generate_file_header("PduR_Cfg.h", "AUTOSAR PDU Router Configuration Header")
        header += """#ifndef PDUR_CFG_H
#define PDUR_CFG_H

/*==================[Includes]==============================================*/
#include "PduR_Types.h"

/*==================[Version Information]===================================*/
#define PDUR_CFG_SW_MAJOR_VERSION    1
#define PDUR_CFG_SW_MINOR_VERSION    0
#define PDUR_CFG_SW_PATCH_VERSION    0

/*==================[Pre-compile Configuration]=============================*/
#define PDUR_DEV_ERROR_DETECT       STD_ON
#define PDUR_VERSION_INFO_API       STD_ON

/*==================[Configuration Constants]===============================*/
#define PDUR_MAX_ROUTING_PATHS      16u
#define PDUR_MAX_PDUS               32u

/*==================[Symbolic Names]========================================*/
#define PduRConf_PduRSrcPdu_PduRSrcPdu_0    0u
#define PduRConf_PduRDestPdu_PduRDestPdu_0  0u

/*==================[External Declarations]=================================*/
extern const PduR_RoutingPathType PduR_RoutingPaths[PDUR_MAX_ROUTING_PATHS];

#endif /* PDUR_CFG_H */
"""
        header_path = self.output_dir / "PduR_Cfg.h"
        header_path.write_text(header, encoding='utf-8')
        files['pdur_header'] = header_path
        
        return files


def main():
    """Main entry point."""
    import argparse
    
    parser = argparse.ArgumentParser(
        description='AUTOSAR ARXML Configuration Generator',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
    %(prog)s --json parsed_config.json --output-dir ./generated
    %(prog)s --arxml config.arxml --output-dir ./generated
        """
    )
    
    parser.add_argument('--json', '-j',
                        help='Input JSON file from ARXML parser')
    parser.add_argument('--arxml', '-a',
                        help='Input ARXML file (will be parsed first)')
    parser.add_argument('--output-dir', '-o', default='./generated',
                        help='Output directory for generated files (default: ./generated)')
    parser.add_argument('--version', '-v', action='version', version='%(prog)s 1.0.0')
    
    args = parser.parse_args()
    
    if not args.json and not args.arxml:
        parser.error('Either --json or --arxml must be specified')
    
    generator = ConfigGenerator(output_dir=args.output_dir)
    
    try:
        if args.json:
            generated = generator.generate_from_json(args.json)
        else:
            # Parse ARXML first
            sys.path.insert(0, str(Path(__file__).parent.parent / 'parser'))
            from arxml_parser import parse_arxml
            
            parser = parse_arxml(args.arxml)
            result = parser.parse_all()
            
            # Save to temp JSON
            temp_json = Path(args.output_dir) / 'temp_parsed.json'
            with open(temp_json, 'w', encoding='utf-8') as f:
                json.dump(result, f, indent=2, default=str)
            
            generated = generator.generate_from_json(str(temp_json))
            temp_json.unlink()
        
        print("\n✅ Configuration Generation Complete!")
        print(f"Output directory: {args.output_dir}")
        print("\nGenerated files:")
        for name, path in generated.items():
            print(f"  ✓ {name}: {path}")
        
        return 0
        
    except Exception as e:
        print(f"\n❌ Error: {e}", file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit(main())
