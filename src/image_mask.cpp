#include "image_mask.h"
#include "utils.h"

#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <set>
#include <QPainter>


Mask Mask::clone() {
	Mask out;
	out.id = id.clone();
	out.color= color.clone();
	return out;
}

MaskDiff::MaskDiff() {}

MaskDiff::MaskDiff(Mask src, Mask dest) {
	id = dest.id - src. id;
	color = dest.color - src.color;
}

Mask MaskDiff::applyDiff(Mask mask) {
	mask.id += id;
	mask.color += color;
	return mask;
}

Mask MaskDiff::removeDiff(Mask mask) {
	std::cout << "MaskDiffId: Size: [" << id.size().height << ", " << id.size().width << ", [" << id.channels() << "]" << std::endl;
	std::cout << "MaskDiffColor: Size: [" << color.size().height << ", " << color.size().width << ", [" << color.channels() << "]" << std::endl;
	std::cout << "MaskId: Size: [" << mask.id.size().height << ", " << mask.id.size().width << ", [" << mask.id.channels() << "]" << std::endl;
	std::cout << "MaskColor: Size: [" << mask.color.size().height << ", " << mask.color.size().width << ", [" << mask.color.channels() << "]" << std::endl;
	mask.id -= id;
	mask.color -= color;

	return mask;
}

ImageMask::ImageMask() {}

ImageMask::ImageMask(const QString &file, Id2Labels id_labels) {
	id = mat2QImage(cv::imread(file.toStdString()));
	color = idToColor(id, id_labels);
    createBuffer();
}

ImageMask::ImageMask(QSize s) {
	id = QImage(s, QImage::Format_RGB888);
	color = QImage(s, QImage::Format_RGB888);
	id.fill(QColor(0, 0, 0));
	color.fill(QColor(0, 0, 0));
    createBuffer();
}

Mask ImageMask::getMask() {
	Mask mask = {};
	mask.id = qImage2Mat(id);
	mask.color = qImage2Mat(color);
	return mask;
}

void ImageMask::setMask(Mask mask) {
	id = mat2QImage(mask.id);
	color = mat2QImage(mask.color);
}

int ImageMask::loadSmartMaskFile(const QString &file) {
	cv::Mat img = cv::imread(file.toStdString());
	color = mat2QImage(img);
	createBuffer();
}

int ImageMask::countInstances() {
	auto colors = findUniqueColors(color);

	// Remove background color as  this is not a valid instance
	colors.erase(QColor(0, 0, 0));
	
	return colors.size();
}

ColorMask ImageMask::getSmartColorMask(ColorMask cm, int instance_num) {
	if (cm.color.red() == 0) {
		return cm;	
	}

	ColorMask smart_cm = {};
	smart_cm.id = cm.id;
	// For debugging to make differences more noticable
	instance_num = instance_num * 10;
	// In the unlikely case that more than 256 * 256 instances have been labelled in this image
	if ((instance_num / (256 * 256)) > 0) {
		std::cout<<"Error: Instance Number("<<instance_num<<") more than allowed 256^2. Looping Smart mask colors"<<std::endl;
		instance_num = instance_num % (256 * 256);
	}

	smart_cm.color = QColor(cm.color.red(), instance_num / 256, instance_num % 256);
	return smart_cm;
}

void ImageMask::drawFillCircle(int x, int y, int pen_size, ColorMask cm) {
	std::cout << "R: " << cm.color.red() << "G: " << cm.color.green() << "B: " << cm.color.blue() <<std::endl;
	QPen pen(QBrush(cm.id), 1.0);
	QPainter painter_id(&id);
	painter_id.setRenderHint(QPainter::Antialiasing, false);
	painter_id.setPen(pen);
	painter_id.setBrush(QBrush(cm.id));
	painter_id.drawEllipse(x, y, pen_size, pen_size);
	painter_id.end();

	QPainter painter_color(&color);
	QPen pen_color(QBrush(cm.color), 1.0);
	painter_color.setRenderHint(QPainter::Antialiasing, false);
	painter_color.setPen(pen_color);
	painter_color.setBrush(QBrush(cm.color));
	painter_color.drawEllipse(x, y, pen_size, pen_size);
	painter_color.end();
    circle(this->_buffer, cv::Point(x,y), pen_size/2.0, getColor(cm.color),CV_FILLED);
}

void ImageMask::fill(int x, int y, ColorMask cm, const Id2Labels & id_labels) {
    //color
    cv::Mat id_mat = qImage2Mat(id);
	cv::floodFill(id_mat, cv::Point(x, y), cv::Scalar(cm.id.red(), cm.id.green(), cm.id.blue()), 0, cv::Scalar(0, 0, 0), cv::Scalar(0, 0, 0));
    //cv::imshow("showing buffer",id_mat);
	id = mat2QImage(id_mat);
	color = idToColor(id, id_labels);
}

void ImageMask::fill(int x, int y, ColorMask cm) {
	cv::Mat color_mat = qImage2Mat(color);
	cv::floodFill(color_mat, cv::Point(x, y), cv::Scalar(cm.color.blue(), cm.color.green(), cm.color.red()), 0, cv::Scalar(0, 0, 0), cv::Scalar(0, 0, 0));
	color = mat2QImage(color_mat);
}

void ImageMask::createBoundingBox(int x, int y){
    
}

void ImageMask::drawBoundingBox(int orig_x, int orig_y, int x, int y){
    
}

void ImageMask::drawPixel(int x, int y, ColorMask cm) {
	id.setPixelColor(x, y, cm.id);
	color.setPixelColor(x, y, cm.color);
}

void ImageMask::updateColor(const Id2Labels & labels) {
	idToColor(id, labels, &color);
}

void ImageMask::exchangeLabel(int x, int y, const Id2Labels& id_labels, ColorMask cm) {

	QColor current_id = id.pixelColor(QPoint(x, y));
	if (current_id.red() == 0 || current_id.green() == 0 || current_id.blue() == 0)
		return;

	cv::Mat id_mat = qImage2Mat(id);
	cv::floodFill(id_mat, cv::Point(x, y), cv::Scalar(cm.id.red(), cm.id.green(), cm.id.blue()), 0, cv::Scalar(0, 0, 0), cv::Scalar(0, 0, 0));

	id = mat2QImage(id_mat);
	color = idToColor(id, id_labels);

}

cv::Scalar ImageMask::getColor(QColor& color){
    int r,g,b;
	color.getRgb(&r, &g, &b);
    return cv::Scalar(b,g,r);
}

void ImageMask::createBuffer(){
    int width = id.width();
    int height = id.height();
    this->_buffer = cv::Mat::zeros(cv::Size(width, height), CV_8UC3);
}

void ImageMask::destroyBuffer(){
    this->_buffer.release(); // will free memory
}