
// include the definition for simplicity:
#include "clang/Interface/Project.h"

//------------------------------------------------------------------------- 

namespace HICFD {
  namespace APP {

//------------------------------------------------------------------------- 
bool loadProject(Project& project, const std::string& filename);
void saveProject(const Project& project, const std::string& filename);

//------------------------------------------------------------------------ 
  } // namespace GUI
} // namespace HICFD
 
//------------------------------------------------------------------------- 
