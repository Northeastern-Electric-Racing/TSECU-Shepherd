#ifndef INC_DCL_H_
#define INC_DCL_H_

#include "datastructs.h"

void dcl_init(void);

void calc_cont_dcl(const analyzer_t* const analyzer, const hv_plate_t* const hv_plate, bms_algos_t* const bms_algos);

#endif /* INC_DCL_H_ */
