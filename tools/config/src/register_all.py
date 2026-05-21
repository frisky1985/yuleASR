"""Unified registration: register ALL BSW modules (30 total)."""

def register_all_modules():
    from mcal_schemas import register_mcal_modules
    from ecual_schemas_1 import register_ecual_part1
    from ecual_schemas_2 import register_ecual_part2
    from services_schemas_1 import register_services_part1
    from remaining_schemas import register_remaining_modules

    register_mcal_modules()
    register_ecual_part1()
    register_ecual_part2()
    register_services_part1()
    register_remaining_modules()
