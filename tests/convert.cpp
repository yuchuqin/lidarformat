/***********************************************************************

This file is part of the LidarFormat project source files.

LidarFormat is an open source library for efficiently handling 3D point
clouds with a variable number of attributes at runtime.


Homepage:

    http://code.google.com/p/lidarformat

Copyright:

    Institut Geographique National & CEMAGREF (2009)

Author:

    Adrien Chauve

Contributors:

        Nicolas David, Olivier Tournaire, Bruno Vallet



    LidarFormat is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    LidarFormat is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with LidarFormat.  If not, see <http://www.gnu.org/licenses/>.

***********************************************************************/

/** \example convert.cpp by Bruno Vallet
 * Converts a lidarformat bin file to ascii
 */
#include <time.h>

#include "config_data_test.h"

#include "LidarFormat/LidarDataContainer.h"
#include "LidarFormat/LidarFile.h"
#include "LidarFormat/geometry/LidarCenteringTransfo.h"

using namespace Lidar;
using namespace std;

int main(int argc, char** argv)
{
    if(argc != 3)
    {
        cout << "Usage: " << argv[0] << " input output" << endl;
        cout << "possible input extentions: .xml, .bin, .txt, .ply, .las" << endl; // terrabin,las
        cout << "possible output extentions: .xml (with binary), .bin (with xml), .txt (with xml), .ply" << endl;
        return 0;
    }

    time_t timer = clock();
    LidarDataContainer ldc(argv[1]);
    ldc.save(argv[2]);
    cout << "Time: " << ( double ) timer/CLOCKS_PER_SEC << " s" << endl;

    return 0;
}



