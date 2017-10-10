/*
 * skiplistdb is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 *
 * Copyright (c) 2017 Partha Susarla <mail@spartha.org>
 *
 */

#include <getopt.h>

#include "skiplistdb.h"

int cmd_backends(int argc __attribute__((unused)), char **argv __attribute__((unused)), const char *progname __attribute__((unused)))
{
        return skiplistdb_backends();
}
