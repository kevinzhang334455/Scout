
namespace scout {

static config<float, 4>::type convert(config<double, 2>::type, config<double, 2>::type)
{
  "_mm_movelh_ps(_mm_cvtpd_ps(%1%), _mm_cvtpd_ps(%2%))";
}

static config<double, 2>::type convert(config<float, 4>::type)
{
  "_mm_cvtps_pd(%1%)";
  "_mm_cvtps_pd(_mm_movehl_ps(%1%, %1%))";
}

}  // namespace scout
