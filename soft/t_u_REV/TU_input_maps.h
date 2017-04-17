#ifndef TU_INPUT_MAPS_H_
#define TU_INPUT_MAPS_H_

#include "TU_input_map.h"

namespace TU {

// {range: 5v, range: 10v} :
const Map input_maps[] = {
  // # slots: 0
  {0, {0, 0}},
  // 1
  {0, {2048, 4095}},
  // 2
  {1, {1024, 2048}},
  // 3
  {2, {683, 1365}},
  // 4
  {3, {512, 1024}},
  // 5
  {4, {409, 819}},
  // 6
  {5, {341, 683}},
  // 7
  {6, {293, 585}},
  // 8
  {7, {256, 512}},
  // 9
  {8, {228, 455}},
  // 10
  {9, {205, 410}},
  // 11
  {10, {186, 372}},
  // 12
  {11, {171, 341}},
  // 13
  {12, {158, 315}},
  // 14
  {13, {146, 293}},
  // 15
  {14, {137, 273}},
  // 16
  {15, {128, 256}},
  // 17
  {0, {0, 0}}
};

}  

namespace TU {

  typedef TU::Map Map;

  class InputMaps {
  public:
    static const Map &GetInputMap(int index) { 
      return input_maps[index]; 
    }
  };
}

#endif  // TU_INPUT_MAPS_H_
