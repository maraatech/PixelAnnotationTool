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
	mask.id -= id;
	mask.color -= color;

	return mask;
}

ImageMask::ImageMask() {}

ImageMask::ImageMask(const QString &file, Id2Labels id_labels) {
	std::cout << "Reading1: " <<  file.toStdString() << std::endl;
	id = mat2QImage(cv::imread(file.toStdString()));
	color = idToColor(id, id_labels);
}

ImageMask::ImageMask(const QString &file, Id2Labels id_labels, LabelInfo class_label) {
	cv::Mat id_mat = cv::imread(file.toStdString());
	// Convert id to correct value
	std::cout << "Label Name: " << class_label.name.toStdString() << std::endl;
	
	for (int i = 0; i < id_mat.rows; i++)
	{
		for (int j = 0; j < id_mat.cols; j++)
		{
			auto pixel = id_mat.at<cv::Vec3b>(i,j);
			
			if (cv::sum(pixel)[0] != 0)
			{
				pixel[2] = class_label.id;
				pixel[1] = class_label.id;
				pixel[0] = class_label.id;
				
				id_mat.at<cv::Vec3b>(i,j) = pixel;
			}
		}
	}
	
	id = mat2QImage(id_mat);
	color = idToColor(id, id_labels);
}

ImageMask::ImageMask(QSize s) {
	id = QImage(s, QImage::Format_RGB888);
	color = QImage(s, QImage::Format_RGB888);
	id.fill(QColor(0, 0, 0));
	color.fill(QColor(0, 0, 0));
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
}

int ImageMask::countInstances() {
    auto colors = findUniqueColors(qImage2Mat(color));

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
	instance_num = instance_num;
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
}

void ImageMask::fill(int x, int y, ColorMask cm, const Id2Labels & id_labels) {
    //color
    cv::Mat id_mat = qImage2Mat(id);
	cv::floodFill(id_mat, cv::Point(x, y), cv::Scalar(cm.id.red(), cm.id.green(), cm.id.blue()), 0, cv::Scalar(0, 0, 0), cv::Scalar(0, 0, 0));

//    cv::imshow("showing buffer",id_mat);
//    cv::waitKey(0);

    id = mat2QImage(id_mat);
    color = idToColor(id, id_labels);
}

void ImageMask::fill(int x, int y, ColorMask cm) {
    cv::Mat color_mat = qImage2Mat(color);
	cv::floodFill(color_mat, cv::Point(x, y), cv::Scalar(cm.color.blue(), cm.color.green(), cm.color.red()), 0, cv::Scalar(0, 0, 0), cv::Scalar(0, 0, 0));
    color = mat2QImage(color_mat);
}

void ImageMask::collapseMask(ImageMask mask)
{
    cv::Mat id_mat = qImage2Mat(id);
    cv::Mat color_mat = qImage2Mat(color);
	
	cv::Mat mask_id = qImage2Mat(mask.id);
	cv::Mat mask_color = qImage2Mat(mask.color);
	
	
	for (int i = 0; i < id_mat.rows; i++)
	{
		for (int j = 0; j < id_mat.cols; j++)
		{
			auto pixel = id_mat.at<cv::Vec3b>(i,j);
			if (pixel[0] == 0 && pixel[1] == 0 && pixel[2] == 0)
			{
				id_mat.at<cv::Vec3b>(i,j) = mask_id.at<cv::Vec3b>(i,j);
				color_mat.at<cv::Vec3b>(i,j) = mask_color.at<cv::Vec3b>(i,j);
			}
		}
	}
	
	id = mat2QImage(id_mat);
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
