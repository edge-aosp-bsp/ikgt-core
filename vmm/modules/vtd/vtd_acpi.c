/*
 * Copyright (c) 2015-2019 Intel Corporation.
 * All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include "dbg.h"
#include "hmm.h"
#include "vtd_acpi.h"
#include "lib/util.h"
#include "modules/acpi.h"

#define DMAR_SIGNATURE 0x52414d44  //the ASCII vaule for "DMAR"

typedef struct {
	acpi_table_header_t header;
	uint8_t             width;
	uint8_t             flags;
	uint8_t             reserved[10];
	uint8_t             remapping_structures[0];
} acpi_dmar_table_t;

typedef struct {
	uint16_t            type;
	uint16_t            length;
	uint8_t             flags;
	uint8_t             reserved;
	uint16_t            segment;
	uint64_t            reg_base_hpa;
	uint8_t             device_scope[0];
} acpi_dma_hw_unit_t;

void vtd_dmar_parse(vtd_engine_t *engine_list)
{
	acpi_dmar_table_t *acpi_dmar;
	acpi_dma_hw_unit_t *unit;
	uint32_t offset = 0, id = 0;
	uint64_t hva;

	D(VMM_ASSERT_EX(engine_list, "engine_list is NULL!\n"));

	acpi_dmar = (acpi_dmar_table_t *)acpi_locate_table(DMAR_SIGNATURE);
	VMM_ASSERT_EX(acpi_dmar, "acpi_dmar is NULL\n");
	print_info("VTD is detected.\n");

	while (offset < (acpi_dmar->header.length - sizeof(acpi_dmar_table_t))) {

		unit = (acpi_dma_hw_unit_t *)(void *)(acpi_dmar->remapping_structures + offset);

		switch(unit->type) {
			/* DMAR type hardware uint */
			case 0:
				VMM_ASSERT_EX((id < DMAR_MAX_ENGINE),
						"too many dmar engines\n");
				VMM_ASSERT_EX(hmm_hpa_to_hva(unit->reg_base_hpa, &hva),
						"fail to convert hpa 0x%llX to hva", unit->reg_base_hpa);
				engine_list[id].reg_base_hpa = unit->reg_base_hpa;
				engine_list[id].reg_base_hva = hva;
				id ++;
				break;

			/* Just take care of DMAR hw uint */
			default:
				break;
		}

		offset += unit->length;
	}
	VMM_ASSERT_EX(id, "No DMAR HW unit found from ACPI table!");

	/* Hide VT-D ACPI table and make a fake one */
	memset((void *)&acpi_dmar->header.signature, 0, acpi_dmar->header.length);
	make_fake_acpi(&acpi_dmar->header);
}

