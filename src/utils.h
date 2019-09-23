#ifndef PIX_ANN_UTILS_H
#define PIX_ANN_UTILS_H

#include "labels.h"
#include "boundingbox.h"

#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include <QImage>
#include <set>

cv::Mat qImage2Mat(QImage const& src);
QImage mat2QImage(cv::Mat const& src);
std::set<QColor> findUniqueColors(QImage const& src);
BoundingBox findBoundingBox(cv::Mat const& src, QColor target, std::string name);
QImage idToColor(const QImage &image_id, const Id2Labels& id_label);
void idToColor(const QImage &image_id, const Id2Labels& id_label, QImage *result);
inline bool operator<(const QColor & a, const QColor & b) { return a.rgb() < b.rgb(); }
QColor invColor(const QColor & color);
QColor readableColor(const QColor & color);
QVector<QColor> colorMap(int size);
cv::Mat convertMat32StoRGBC3(const cv::Mat &mat);
QImage watershed(const QImage& qimage, const QImage & qmarkers_mask);
QImage removeBorder(const QImage & mask_id, const Id2Labels & labels, cv::Size win_size = cv::Size(3,3));
bool isFullZero(const QImage& image);
int rgbToInt(uchar r, uchar g, uchar b);
void intToRgb(int value, uchar &r, uchar &g, uchar &b);
unsigned char random_char();
std::string generate_hex(const unsigned int len);
#endif
