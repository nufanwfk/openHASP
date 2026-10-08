#pragma once
#define GPIO_IS_VALID_GPIO(x) ((x) >= 0 && (x) < 49)
#define GPIO_IS_VALID_OUTPUT_GPIO(x) (GPIO_IS_VALID_GPIO(x) && (x) != 46)
