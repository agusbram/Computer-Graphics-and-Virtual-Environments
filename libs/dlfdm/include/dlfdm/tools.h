#ifndef DLFDM_TOOLS_H
#define DLFDM_TOOLS_H

namespace dlfdm {

template <class T>
inline T clamp(const T& value, const T& min, const T& max){
    if(value < min){
        return min;
    }
    else if (value > max) {
        return max;
    }
    else {
        return value;
    }
}

} // namespace dlfdm

#endif // DLFDM_TOOLS_H
