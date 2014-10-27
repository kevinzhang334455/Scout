
namespace scout {

static void vectorized_loop_prolog()
{
  "SMDB_LOOP_START";
}

static void vectorized_loop_epilog()
{
  "SMDB_LOOP_END";
}

static void vectorized_function_prolog()
{
  "SMDB_FN_START";
}

static void vectorized_function_epilog()
{
  "SMDB_FN_END";
}

}  // namespace scout
