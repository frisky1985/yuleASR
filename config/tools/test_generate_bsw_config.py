#!/usr/bin/env python3
"""Regression tests for the repository BSW configuration generator."""

import copy
import importlib.util
import tempfile
import unittest
from pathlib import Path


GENERATOR_PATH = Path(__file__).resolve().with_name("generate_bsw_config.py")
SPEC = importlib.util.spec_from_file_location("generate_bsw_config", GENERATOR_PATH)
GENERATOR = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(GENERATOR)


class GenerateBswConfigTest(unittest.TestCase):
    def semantic_errors(self, cfg):
        validate = getattr(GENERATOR, "validate_semantics", None)
        self.assertTrue(callable(validate), "validate_semantics must be implemented")
        return validate(cfg)

    def test_module_name_supports_linkable_configuration_templates(self):
        self.assertEqual(
            GENERATOR.module_name_from_template(Path("Com_Cfg.h.j2")), "Com"
        )
        self.assertEqual(
            GENERATOR.module_name_from_template(Path("Com_Lcfg.c.j2")), "Com"
        )
        self.assertEqual(
            GENERATOR.module_name_from_template(Path("Dcm_Lcfg.c.j2")), "Dcm"
        )

    def test_com_explicit_ipdu_ids_are_separate_from_capacity(self):
        cfg = GENERATOR.load_json(GENERATOR.DEFAULT_CONFIG)
        com = cfg["modules"]["Com"]

        self.assertEqual(com["ipdus"], 32)
        self.assertEqual(com["ipdu_ids"], [0, 1, 2, 3, 4])

    def test_dcm_protocols_use_only_production_pdur_routes(self):
        cfg = GENERATOR.load_json(GENERATOR.DEFAULT_CONFIG)

        self.assertEqual(
            cfg["modules"]["Dcm"]["protocols"],
            [
                {
                    "name": "OBD_ON_CAN",
                    "id": 0,
                    "rx_pdu_id": 6,
                    "tx_pdu_id": 5,
                },
            ],
        )
        self.assertEqual(cfg["modules"]["Dcm"]["counts"]["protocols"], 2)

    def test_pdur_explicit_routes_and_groups_match_production_configuration(self):
        cfg = GENERATOR.load_json(GENERATOR.DEFAULT_CONFIG)
        pdur = cfg["modules"]["PduR"]

        self.assertEqual(pdur["routing_paths"], 16)
        self.assertEqual(pdur["routing_path_groups"], 4)
        self.assertIn("paths", pdur)
        self.assertIn("groups", pdur)
        self.assertEqual(
            [
                (
                    path["name"],
                    path["source"]["pdu_id"],
                    path["source"]["module"],
                    path["source"]["sdu_length"],
                    path["destinations"][0]["pdu_id"],
                    path["destinations"][0]["module"],
                )
                for path in pdur["paths"]
            ],
            [
                ("EngineStatusTx", 0, "COM", 8, 0, "CANIF"),
                ("VehicleSpeedTx", 1, "COM", 8, 1, "CANIF"),
                ("BatteryStatusTx", 2, "COM", 8, 2, "CANIF"),
                ("EngineCmdRx", 3, "CANIF", 8, 3, "COM"),
                ("VehicleCmdRx", 4, "CANIF", 8, 4, "COM"),
                ("DiagResponseTx", 5, "DCM", 8, 5, "CANIF"),
                ("DiagRequestRx", 6, "CANIF", 8, 6, "DCM"),
            ],
        )
        self.assertTrue(
            all(
                len(path["destinations"]) == 1
                and path["path_type"] == "DIRECT"
                and path["gateway_operation"] is False
                for path in pdur["paths"]
            )
        )
        self.assertEqual(
            pdur["groups"],
            [
                {"id": 0, "pdu_ids": [0, 1, 2], "default_enabled": True},
                {"id": 1, "pdu_ids": [3, 4], "default_enabled": True},
                {"id": 2, "pdu_ids": [5, 6], "default_enabled": True},
                {"id": 3, "pdu_ids": [], "default_enabled": False},
            ],
        )

    def test_pdur_schema_requires_complete_route_elements(self):
        cfg = copy.deepcopy(GENERATOR.load_json(GENERATOR.DEFAULT_CONFIG))
        del cfg["modules"]["PduR"]["paths"][0]["source"]["pdu_id"]

        valid, errors = GENERATOR.validate_config(cfg, GENERATOR.DEFAULT_SCHEMA)

        self.assertFalse(valid)
        self.assertTrue(any("pdu_id" in error for error in errors))

    def test_semantic_validation_accepts_production_configuration(self):
        cfg = GENERATOR.load_json(GENERATOR.DEFAULT_CONFIG)

        self.assertEqual(self.semantic_errors(cfg), [])

    def test_semantic_validation_rejects_duplicate_identifiers(self):
        mutations = {
            "Com.ipdu_ids": lambda cfg: cfg["modules"]["Com"]["ipdu_ids"].append(0),
            "PduR.paths.name": lambda cfg: cfg["modules"]["PduR"]["paths"][1].update(
                name=cfg["modules"]["PduR"]["paths"][0]["name"]
            ),
            "PduR.paths.source": lambda cfg: cfg["modules"]["PduR"]["paths"][1]["source"].update(
                pdu_id=cfg["modules"]["PduR"]["paths"][0]["source"]["pdu_id"],
                module=cfg["modules"]["PduR"]["paths"][0]["source"]["module"],
            ),
            "PduR.groups.id": lambda cfg: cfg["modules"]["PduR"]["groups"][1].update(
                id=cfg["modules"]["PduR"]["groups"][0]["id"]
            ),
            "Dcm.protocols.id": lambda cfg: cfg["modules"]["Dcm"]["protocols"].append(
                {"name": "DUPLICATE", "id": 0, "rx_pdu_id": 6, "tx_pdu_id": 5}
            ),
        }
        for expected, mutate in mutations.items():
            with self.subTest(expected=expected):
                cfg = copy.deepcopy(GENERATOR.load_json(GENERATOR.DEFAULT_CONFIG))
                mutate(cfg)
                self.assertTrue(
                    any(expected in error for error in self.semantic_errors(cfg))
                )

    def test_semantic_validation_enforces_explicit_element_capacities(self):
        mutations = {
            "Com.ipdus": lambda cfg: cfg["modules"]["Com"].update(ipdus=4),
            "PduR.routing_paths": lambda cfg: cfg["modules"]["PduR"].update(routing_paths=6),
            "PduR.routing_path_groups": lambda cfg: cfg["modules"]["PduR"].update(
                routing_path_groups=3
            ),
            "Dcm.counts.protocols": lambda cfg: cfg["modules"]["Dcm"]["counts"].update(
                protocols=0
            ),
        }
        for expected, mutate in mutations.items():
            with self.subTest(expected=expected):
                cfg = copy.deepcopy(GENERATOR.load_json(GENERATOR.DEFAULT_CONFIG))
                mutate(cfg)
                self.assertTrue(
                    any(expected in error for error in self.semantic_errors(cfg))
                )

    def test_semantic_validation_checks_com_route_endpoints(self):
        mutations = [
            lambda cfg: cfg["modules"]["PduR"]["paths"][0]["source"].update(pdu_id=99),
            lambda cfg: cfg["modules"]["PduR"]["paths"][3]["destinations"][0].update(
                pdu_id=99
            ),
        ]
        for mutate in mutations:
            with self.subTest(mutate=mutate):
                cfg = copy.deepcopy(GENERATOR.load_json(GENERATOR.DEFAULT_CONFIG))
                mutate(cfg)
                self.assertTrue(
                    any("Com.ipdu_ids" in error for error in self.semantic_errors(cfg))
                )

    def test_semantic_validation_checks_dcm_protocol_pdur_references(self):
        cfg = copy.deepcopy(GENERATOR.load_json(GENERATOR.DEFAULT_CONFIG))
        cfg["modules"]["Dcm"]["protocols"][0].update(rx_pdu_id=99, tx_pdu_id=98)

        errors = self.semantic_errors(cfg)

        self.assertTrue(any("Dcm.protocols[0].rx_pdu_id" in error for error in errors))
        self.assertTrue(any("Dcm.protocols[0].tx_pdu_id" in error for error in errors))

    def test_semantic_validation_rejects_dcm_duplicate_pdu_endpoints(self):
        cfg = copy.deepcopy(GENERATOR.load_json(GENERATOR.DEFAULT_CONFIG))
        cfg["modules"]["Dcm"]["protocols"].append(
            {"name": "SECONDARY", "id": 1, "rx_pdu_id": 6, "tx_pdu_id": 5}
        )

        errors = self.semantic_errors(cfg)

        self.assertTrue(any("Dcm.protocols.rx_pdu_id" in error for error in errors))
        self.assertTrue(any("Dcm.protocols.tx_pdu_id" in error for error in errors))

    def test_semantic_validation_requires_dense_dcm_protocol_ids(self):
        cfg = copy.deepcopy(GENERATOR.load_json(GENERATOR.DEFAULT_CONFIG))
        cfg["modules"]["Dcm"]["protocols"][0]["id"] = 1

        errors = self.semantic_errors(cfg)

        self.assertTrue(any("Dcm.protocols[0].id" in error for error in errors))

    def test_semantic_validation_checks_group_route_references(self):
        cfg = copy.deepcopy(GENERATOR.load_json(GENERATOR.DEFAULT_CONFIG))
        cfg["modules"]["PduR"]["groups"][0]["pdu_ids"][0] = 99

        errors = self.semantic_errors(cfg)

        self.assertTrue(any("PduR.groups[0].pdu_ids[0]" in error for error in errors))

    def test_validate_only_cli_runs_semantic_validation(self):
        cfg = copy.deepcopy(GENERATOR.load_json(GENERATOR.DEFAULT_CONFIG))
        cfg["modules"]["Dcm"]["protocols"].append(
            {"name": "DUPLICATE", "id": 0, "rx_pdu_id": 6, "tx_pdu_id": 5}
        )
        import json

        with tempfile.TemporaryDirectory() as tmp:
            config_path = Path(tmp) / "bsw_config.json"
            config_path.write_text(json.dumps(cfg), encoding="utf-8")
            result = GENERATOR.main(
                [
                    "--config",
                    str(config_path),
                    "--schema",
                    str(GENERATOR.DEFAULT_SCHEMA),
                    "--validate-only",
                ]
            )

        self.assertEqual(result, 1)

    def test_dcm_lcfg_is_generated_from_protocol_mappings(self):
        cfg = GENERATOR.load_json(GENERATOR.DEFAULT_CONFIG)
        env = GENERATOR.build_environment(GENERATOR.DEFAULT_TEMPLATES)
        templates = GENERATOR.discover_templates(GENERATOR.DEFAULT_TEMPLATES)

        with tempfile.TemporaryDirectory() as tmp:
            output_dir = Path(tmp)
            _, _, ok = GENERATOR.generate(
                env=env,
                templates=templates,
                cfg=cfg,
                output_dir=output_dir,
                module_filter=["Dcm"],
                write=True,
            )

            self.assertTrue(ok)
            output_path = output_dir / "Dcm_Lcfg.c"
            self.assertTrue(output_path.is_file())
            generated = output_path.read_text(encoding="utf-8")
            self.assertIn(".ProtocolId = 0U", generated)
            self.assertIn(".RxPduId = 6U", generated)
            self.assertIn(".TxPduId = 5U", generated)
            self.assertNotIn(".ProtocolId = 1U", generated)
            self.assertIn(".NumProtocols = DCM_NUM_PROTOCOLS", generated)
            self.assertIn(".Protocols = Dcm_Protocols", generated)
            generated_cfg = (output_dir / "Dcm_Cfg.h").read_text(encoding="utf-8")
            self.assertIn("#define DCM_NUM_PROTOCOLS    (1U)", generated_cfg)

    def test_pdur_lcfg_is_generated_from_explicit_routes_and_groups(self):
        cfg = GENERATOR.load_json(GENERATOR.DEFAULT_CONFIG)
        env = GENERATOR.build_environment(GENERATOR.DEFAULT_TEMPLATES)
        templates = GENERATOR.discover_templates(GENERATOR.DEFAULT_TEMPLATES)

        with tempfile.TemporaryDirectory() as tmp:
            output_dir = Path(tmp)
            _, _, ok = GENERATOR.generate(
                env=env,
                templates=templates,
                cfg=cfg,
                output_dir=output_dir,
                module_filter=["PduR"],
                write=True,
            )

            self.assertTrue(ok)
            output_path = output_dir / "PduR_Lcfg.c"
            self.assertTrue(output_path.is_file())
            generated = output_path.read_text(encoding="utf-8")
            self.assertEqual(generated.count(".SourcePduId ="), 7)
            self.assertEqual(generated.count(".DestPdus ="), 7)
            self.assertEqual(generated.count(".GroupId ="), 4)
            for pdu_id in range(7):
                self.assertIn(f".SourcePduId = (PduIdType){pdu_id}U", generated)
            self.assertIn(".NumRoutingPaths = 7U", generated)
            self.assertIn(".NumRoutingPathGroups = 4U", generated)
            self.assertIn(".PduIds = NULL_PTR", generated)
            self.assertIn(".NumPduIds = 0U", generated)

    def test_com_lcfg_is_generated_from_explicit_ipdu_ids(self):
        cfg = GENERATOR.load_json(GENERATOR.DEFAULT_CONFIG)
        env = GENERATOR.build_environment(GENERATOR.DEFAULT_TEMPLATES)
        templates = GENERATOR.discover_templates(GENERATOR.DEFAULT_TEMPLATES)

        with tempfile.TemporaryDirectory() as tmp:
            output_dir = Path(tmp)
            _, _, ok = GENERATOR.generate(
                env=env,
                templates=templates,
                cfg=cfg,
                output_dir=output_dir,
                module_filter=["Com"],
                write=True,
            )

            self.assertTrue(ok)
            generated = (output_dir / "Com_Lcfg.c").read_text(encoding="utf-8")
            self.assertIn("Com_Signals[COM_MAX_SIGNALS]", generated)
            for pdu_id in range(5):
                self.assertIn(f".PduId = {pdu_id}U", generated)
            self.assertIn(".NumIPdus = 5U", generated)
            self.assertIn(".DataLength = 8U", generated)
            self.assertIn(".TimePeriod = 100U", generated)


if __name__ == "__main__":
    unittest.main()
