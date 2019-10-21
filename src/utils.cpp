#include "utils.h"

//-------------------------------------------------------------------------------------------------------------
QImage mat2QImage(cv::Mat const& src) {
	cv::Mat temp; // make the same cv::Mat
	cv::cvtColor(src, temp, cv::COLOR_BGR2RGB); // cvtColor Makes a copt, that what i need
	QImage dest((const uchar *)temp.data, temp.cols, temp.rows,int(temp.step), QImage::Format_RGB888);
	dest.bits(); // enforce deep copy, see documentation 
				 // of QImage::QImage ( const uchar * data, int width, int height, Format format )
	return dest;
}
cv::Mat qImage2Mat(QImage const& src) {
	cv::Mat tmp(src.height(), src.width(), CV_8UC3, (uchar*)src.bits(), src.bytesPerLine());
	cv::Mat result; // deep copy just in case (my lack of knowledge with open cv)
	cv::cvtColor(tmp, result, cv::COLOR_BGR2RGB);
	return result;
}

std::set<QColor> findUniqueColors(QImage const& src) {
	cv::Mat img = qImage2Mat(src);
	// Count unique colors
	std::set<QColor> unique;

	for (int i = 0; i < img.rows; i++)
	{
		for (int j = 0; j < img.cols; j++)
		{
			auto pixel = img.at<cv::Vec3b>(i,j);
			QColor color = QColor(pixel[2], pixel[ 1], pixel[0]);
			unique.insert(color);
		}
	}

	return unique;
}

BoundingBox findBoundingBox(cv::Mat const& img, QColor target, std::string name) {
	// (x1, y1) -------> (x2, y2)
	// Where X = Cols and Y = Rows

	cv::Point minmin = cv::Point(0, 0);
	cv::Point maxmax = cv::Point(img.cols - 1, img.rows - 1);

	// Top down scan
	for (int i = minmin.y; i <= maxmax.y; i++)
	{
		minmin.y = i;
		for (int j = minmin.x; j <= maxmax.x; j++)
		{
			auto pixel = img.at<cv::Vec3b>(i,j);
			if (target == QColor(pixel[2], pixel[1], pixel[0])) {
				goto finish_top_scan;
			}
		}
	}
	finish_top_scan:

	// Bottom up scan
	for (int i = maxmax.y; i >= minmin.y; i--)
	{
		maxmax.y = i;
		for (int j = minmin.x; j <= maxmax.x; j++)
		{
			auto pixel = img.at<cv::Vec3b>(i,j);
			if (target == QColor(pixel[2], pixel[1], pixel[0])) {
				goto finish_bottom_scan;
			}
		}
	}
	finish_bottom_scan:

	// Left right scan
	for (int j = minmin.x; j <= maxmax.x; j++)
	{
		minmin.x = j;
		for (int i = minmin.y; i <= maxmax.y; i++)
		{
			auto pixel = img.at<cv::Vec3b>(i,j);
			if (target == QColor(pixel[2], pixel[1], pixel[0])) {
				goto finish_left_scan;
			}
		}
	}
	finish_left_scan:

	// Right left scan
	for (int j = maxmax.x; j >= minmin.x; j--)
	{
		maxmax.x = j;
		for (int i = minmin.y; i <= maxmax.y; i++)
		{
			auto pixel = img.at<cv::Vec3b>(i,j);
			if (target == QColor(pixel[2], pixel[1], pixel[0])) {
				goto finish_right_scan;
			}
		}
	}
	finish_right_scan:

	return BoundingBox(minmin, maxmax, name);
}

QImage idToColor(const QImage &image_id, const Id2Labels& id_label) {
	QImage result(image_id.size(), QImage::Format_RGB888);
	idToColor(image_id, id_label, &result);
	return result;
}

void idToColor(const QImage &image_id, const Id2Labels& id_label, QImage *result) {
	int id = 0;
	uchar * pix;

	for (int y = 0; y < image_id.height(); y++) {
		const uchar * line_in = image_id.scanLine(y);
		uchar * line_out = result->scanLine(y);
		for (int x = 0; x < image_id.width()*3; x+=3) {
			int id = line_in[x];
	
			QMap<int, const LabelInfo*>::const_iterator it = id_label.find(id);
			if (it != id_label.end()) {
				pix = &line_out[x];
				const LabelInfo * label = *it;
				*pix = label->color.red(); pix++;
				*pix = label->color.green(); pix++;
				*pix = label->color.blue();
            } else {
                pix = &line_out[x];
                pix[0] = 255;
                pix[1] = 255;
                pix[2] = 255;
            }
		}
	}
}

QColor readableColor(const QColor & color)
{
	int r, g, b;
	color.getRgb(&r, &g, &b);

	if ((r*0.299 + g*0.587 + b*0.114) > 150)
		return QColor(0, 0, 0);

	return QColor(255, 255, 255);

}
QColor invColor(const QColor& color) {
	int h, s, v;
	color.getHsv(&h, &s, &v);
	h = (h + 180) % 360;
	s = 255 - s;
	v = 255 - v;
	QColor color_inv;
	color_inv.setHsv(h, s, v);
	return color_inv;
}

int rgbToInt(uchar r, uchar g, uchar b) {
	return (r<<16) + (g<<8) + b;
}

void intToRgb(int value,uchar &r, uchar &g, uchar &b) {
	r = (value >> 16) & 0x000f;
	g = (value >>  8) & 0x000f;
	b = (value >>  0) & 0x000f;
}

QVector<QColor> colorMap(int size) {
	double e = 300. / size;
	double h = 0.;
	double s = 255.;
	double v = 255.;
	QVector<QColor> res;
	for (size_t i = 0; i < size; i++) {
		QColor hsv;
		hsv.setHsv(h, s, v);
		res.push_back(QColor(hsv.rgb()));
		h += e;
	}
	return res;
}

cv::Mat convertMat32StoRGBC3(const cv::Mat& mat) {
	cv::Mat dst(mat.size(), CV_8UC3);
	uchar *pix;
	uchar label;
	for (int r = 0; r < dst.rows; ++r) {
		const int* ptr = mat.ptr<int>(r);
		uchar* ptr_dst = dst.ptr<uchar>(r);
		for (int c = 0; c < dst.cols; ++c) {
			label = ptr[c];
			pix = &ptr_dst[c*3];
			if (label > 0) {
				pix[0] = label;
				pix[1] = label;
				pix[2] = label;
			} else {
				pix[0] = 0;
				pix[1] = 0;
				pix[2] = 0;
			}
		}
	}
	return dst;
}

QImage removeBorder(const QImage & mask_id, const Id2Labels & labels, cv::Size win_size) {
	QImage result = mask_id.copy();

	for (int y = 1; y < mask_id.height() - 1; y++) {
		const uchar * line_curr = mask_id.scanLine(y);
		uchar * line_out = result.scanLine(y);
		for (int X = 1; X < mask_id.width() - 1; X++) {
			int x = X * 3;
			int id = line_curr[x];
			if (labels.find(id) == labels.end()) {
				std::map<int, int> mapk;
				for (int yy = -(win_size.height >> 1); yy <= win_size.height >> 1; yy++) {
					const uchar * l_curr = mask_id.scanLine(y + yy);
					for (int xx = -(win_size.width >> 1); xx <= win_size.width >> 1; xx++) {
						if (yy == 0 && xx == 0) continue;
						if (mapk.find(l_curr[x+xx]) != mapk.end()) mapk[l_curr[x + xx]] ++;
						else mapk[l_curr[x + xx]] = 1;
					}
				}
				int id_max = 0;
				int id_resul = mapk.begin()->first;
				std::map<int, int>::iterator it = mapk.begin();
				while (it != mapk.end()) {
					if (it->first != 255) {
						if (it->second > id_max) {
							id_max = it->second;
							id_resul = it->first;
						}
					}
					it++;
				}
				line_out[x] = id_resul;
				line_out[x + 1] = id_resul;
				line_out[x + 2] = id_resul;
			}
		}
	}
	return result;
}

bool isFullZero(const QImage& image) {
	for (int y = 0; y < image.height(); y++) {
		const uchar * line = image.scanLine(y);
		for (int x = 0; x < image.bytesPerLine(); x++) {
			if (line[x] > 0)
				return false;
		}
	}
	return true;
}
//-------------------------------------------------------------------------------------------------------------

QImage floodfill(const QImage& qimage, const QImage & qmarkers_mask, cv::Point point) {
	cv::Mat image = qImage2Mat(qimage);
	cv::Mat markers_mask = qImage2Mat(qmarkers_mask);
	cv::Mat markers = cv::Mat::zeros(markers_mask.size(), CV_32S);
    cv::Mat new_mask = convertMat32StoRGBC3(markers);
	return mat2QImage(new_mask);
}

#include <vector>
#include <iostream>
#include <sstream>
#include <random>
#include <climits>
#include <algorithm>
#include <functional>
#include <string>

unsigned char random_char() {
    std::random_device rd;
    std::mt19937 gen(rd()); 
    std::uniform_int_distribution<> dis(0, 255);
    return static_cast<unsigned char>(dis(gen));
}

std::string generate_hex(const unsigned int len) {
    std::stringstream ss;
    for(auto i = 0; i < len; i++) {
        auto rc = random_char();
        std::stringstream hexstream;
        hexstream << std::hex << int(rc);
        auto hex = hexstream.str(); 
        ss << (hex.length() < 2 ? '0' + hex : hex);
    }        
    return ss.str();
}
