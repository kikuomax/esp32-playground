#ifndef _UTILS_H
#define _UTILS_H

/**
 * @file utils.h
 *
 * Utilities.
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MAX
/**
 * @brief Maximum of given two values.
 *
 * @param[in] x
 *
 *   Value to compare.
 *
 * @param[in] y
 *
 *   Another value to compare.
 *
 * @return
 *
 *   Bigger value of `x` and `y`.
 */
#define MAX(x, y)  ((x) > (y) ? (x) : (y))
#endif

#ifndef MIN
/**
 * @brief Minimum of given two values.
 *
 * @param[in] x
 *
 *   Value to compare.
 *
 * @param[in] y
 *
 *   Another value to compare.
 *
 * @return
 *
 *   Smaller value of `x` and `y`.
 */
#define MIN(x, y)  ((x) < (y) ? (x) : (y))
#endif

#ifdef __cplusplus
}
#endif

#endif
