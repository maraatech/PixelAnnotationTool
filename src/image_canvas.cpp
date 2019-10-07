
#include "image_canvas.h"
#include "main_window.h"

#include <QtDebug>
#include <QtWidgets>
#include <fstream>
#include <QtXml>

ImageCanvas::ImageCanvas(MainWindow *ui) :
    QLabel() ,
	_ui(ui){

    _scroll_parent = new QScrollArea(ui);
    setParent(_scroll_parent);
	resize(800,600);
	_scale = _ui->spinbox_scale->value();
	_alpha = _ui->spinbox_alpha->value();
	_pen_size = _ui->spinbox_pen_size->value();
	_initPixmap();
	setScaledContents(true);
	setMouseTracking(true);
	_button_is_pressed = false;

    _scroll_parent->setBackgroundRole(QPalette::Dark);
    _scroll_parent->setWidget(this);
    _operation_mode = DRAW_MODE;
    _instance_num = 0;

    _op_manager = new OperationManager(this);
}

ImageCanvas::~ImageCanvas() {
    _scroll_parent->deleteLater();
}

bool ImageCanvas::isNotSaved() { 
    return _op_manager->num_ops() > 0; 
}

void ImageCanvas::_initPixmap() {
	QPixmap newPixmap = QPixmap(width(), height());
	newPixmap.fill(Qt::white);
	QPainter painter(&newPixmap);
	const QPixmap * p = pixmap();
	if (p != NULL)
		painter.drawPixmap(0, 0, *pixmap());
	painter.end();
	setPixmap(newPixmap);
}

void ImageCanvas::loadImage(const QString &filename) {
	if (!_image.isNull() )
		saveMask();

	_img_file = filename;
	QFileInfo file(_img_file);
	if (!file.exists()) return;

	_image = mat2QImage(cv::imread(_img_file.toStdString()));
    _orig_image = _image.copy();
	
	_mask_file = file.dir().absolutePath()+ "/" + file.baseName() + "_mask.png";
    _smart_mask_file = file.dir().absolutePath()+ "/" + file.baseName() + "_smart_mask.png";
    _annotation_file = file.dir().absolutePath()+ "/xml/" + file.baseName() + ".xml";
    
	if (QFile(_mask_file).exists()) {
		_mask = ImageMask(_mask_file,_ui->id_labels);
        _smart_mask = ImageMask(_image.size());
        _smart_mask.loadSmartMaskFile(_smart_mask_file);
        _instance_num = _smart_mask.countInstances();
        std::cout << "Found Unique Instances:" << _instance_num << std::endl;
		_ui->checkbox_manuel_mask->setChecked(true);
	} else {
		clearMask();
	}
    parseXML(_annotation_file);
	_ui->undo_action->setEnabled(false);
	_ui->redo_action->setEnabled(false);
    
	setPixmap(QPixmap::fromImage(_image));
	resize(_scale *_image.size());
    redrawBoundingBox();
}

void ImageCanvas::parseXML(QString file_name){
    // Create a document to write XML
    QDomDocument document;
    // Open a file for reading
    QFile file(file_name);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        std::cout << "cant find file."<<file_name.toStdString()<<std::endl;
        return;
    }else{
        // loading
        if(!document.setContent(&file))
        {
            std::cout << "Failed to load the file for reading.";
            return;
        }
        file.close();
    }
    // Getting root element
    QDomElement root = document.firstChildElement();
    QDomNodeList nodes = root.elementsByTagName("object");
    for(int i = 0; i < nodes.count(); i++){
        QDomElement boundbox_node = nodes.at(i).toElement().elementsByTagName("bndbox").at(0).toElement();
        QString name = boundbox_node.elementsByTagName("name").at(0).firstChild().toText().data();
        int min_x = boundbox_node.elementsByTagName("xmin").at(0).firstChild().toText().data().toInt();
        int max_x = boundbox_node.elementsByTagName("xmax").at(0).firstChild().toText().data().toInt();
        int min_y = boundbox_node.elementsByTagName("ymin").at(0).firstChild().toText().data().toInt();
        int max_y = boundbox_node.elementsByTagName("ymax").at(0).firstChild().toText().data().toInt();

        int color_r = boundbox_node.elementsByTagName("mask_r").at(0).firstChild().toText().data().toInt();
        int color_g = boundbox_node.elementsByTagName("mask_g").at(0).firstChild().toText().data().toInt();
        int color_b = boundbox_node.elementsByTagName("mask_b").at(0).firstChild().toText().data().toInt();

        auto bbox = BoundingBox(cv::Point(min_x,min_y),cv::Point(max_x, max_y),name.toStdString());
        bbox.setMaskColor(QColor(color_r, color_g, color_b));

        box_list.push_back(bbox);
    }
    return;
}

void ImageCanvas::smartMask() {
    std::cout << "Entering Smark Mask" << std::endl;

    int idx = getSelectedBox();
    
    if (idx != -1)
    {
        for (auto bbox : box_list) {
            bbox.unselect();
        }
    }

    //TODO: Performance improvement, could just compare buffers to see if changes have been made
    cv::Mat img = qImage2Mat(_smart_mask.color);

    auto unique = findUniqueColors(_smart_mask.color);
    unique.erase(QColor(0, 0, 0));
    
    int n = unique.size();
    if (n > _instance_num) {
        std::cout << "Updated instance number to " << n << std::endl;
    }

    _instance_num = n;

    auto preop_box_list = box_list;

    box_list.clear();

    std::cout << "Found unique colors: " << n << std::endl;
    for (auto c : unique)
    {
        int id = _ui->id_labels.getIdFromR(c.red());
        auto label = _ui->id_labels[id]->name.toStdString();

        BoundingBox bbox = findBoundingBox(img, c, label);
        bbox.setMaskColor(c);
        box_list.push_back(bbox);
    }

    auto postop_box_list = box_list;
    _op_manager->smart_mask(preop_box_list, postop_box_list);

    redrawBoundingBox();
}


void ImageCanvas::saveMask() {
	if (isFullZero(_mask.id))
		return;

	_mask.id.save(_mask_file);
    QFileInfo file(_img_file);

    QString color_file = file.dir().absolutePath() + "/" + file.baseName() + "_color_mask.png";
    _mask.color.save(color_file);
    _smart_mask.color.save(_smart_mask_file);
    saveAnnotation();
    _ui->setStarAtNameOfTab(false);
}

void ImageCanvas::saveAnnotation(){
    QFileInfo file(_img_file);
    //create text
    std::string text ="<annotation>\n\
	<folder>0</folder>\n\
	<filename>"+ file.baseName().toStdString()+"</filename>\n\
	<path>"+file.dir().absolutePath().toStdString() + "/" + file.baseName().toStdString()+"</path>\n\
	<source>\n\
	\t<database>Unknown</database>\n\
	</source>\n\
	<size>\n\
	\t<width>"+std::to_string(width())+"</width>\n\
	\t<height>"+std::to_string(height())+"</height>\n\
	\t<depth>3</depth>\n\
	</size>\n\
    <segmented>0</segmented>\n";
    for(BoundingBox b:box_list){
        text+=b.toXML();
    }
    text = text+ "\n</annotation>";
    std::string file_name = file.dir().absolutePath().toStdString() + "/xml/"+file.baseName().toStdString()+"_bbox.xml";
    std::ofstream out(file_name);
    out << text;
    out.close();
    std::cout<<"saving xml"<<file_name<<std::endl;
}

void ImageCanvas::scaleChanged(double scale) {
	_scale  = scale ;
	resize(_scale * _image.size());
	repaint();
}

void ImageCanvas::alphaChanged(double alpha) {
	_alpha = alpha;
	repaint();
}

void ImageCanvas::paintEvent(QPaintEvent *event) {
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing, false);
	QRect rect = painter.viewport();
	QSize size = _scale * _image.size();
	if (size != _image.size()) {
		rect.size().scale(size, Qt::KeepAspectRatio);
		painter.setViewport(rect.x(), rect.y(), size.width(), size.height());
		painter.setWindow(pixmap()->rect());
	}
	painter.drawImage(QPoint(0, 0), _image);
	painter.setOpacity(_alpha);

	if (!_mask.id.isNull() && _ui->checkbox_manuel_mask->isChecked()) {
		painter.drawImage(QPoint(0, 0), _mask.color);
	}

	if (_mouse_pos.x() > 10 && _mouse_pos.y() > 10 && 
		_mouse_pos.x() <= QLabel::size().width()-10 &&
		_mouse_pos.y() <= QLabel::size().height()-10) {
		painter.setBrush(QBrush(_color.color));
		painter.setPen(QPen(QBrush(_color.color), 1.0));
		painter.drawEllipse(_mouse_pos.x() / _scale - _pen_size / 2, _mouse_pos.y() / _scale - _pen_size / 2, _pen_size, _pen_size);
		painter.end();
	}
}

cv::Point ImageCanvas::getXYonImage(QMouseEvent *e){
    return getXYonImage(e->x(), e->y());
}

cv::Point ImageCanvas::getXYonImage(int x_gui, int y_gui){
    int x =0;
    int y =0;
    if (_pen_size > 0) {
		x = x_gui / _scale - _pen_size / 2;
		y = y_gui / _scale - _pen_size / 2;
	} else {
		x = (x_gui+0.5) / _scale ;
		y = (y_gui+0.5) / _scale ;
	}
    return cv::Point(x,y);
}


int ImageCanvas::getSelectedBox(){
    for(int i =0;i<box_list.size();i++){
        if(box_list[i].is_selected()){
            return i;
        }
    }
    return -1;
}

//remember e is xy of gui
//xy should be converted of the image

void ImageCanvas::drawBoundingBox(BoundingBox b){
    cv::Mat _image_mat = qImage2Mat(_image);
    b.draw(_image_mat);
    _image = mat2QImage(_image_mat);
    update();
}

void ImageCanvas::drawMarkedBoundingBox(BoundingBox b){
    cv::Mat _image_mat = qImage2Mat(_image);
    b.draw_marked(_image_mat);
    _image = mat2QImage(_image_mat);
    update();
}

void ImageCanvas::mousePressEvent(QMouseEvent * e) {
	setFocus();
    cv::Point p = getXYonImage(e);
    std::cout<<"clicked"<<p<<" operation:"<<_operation_mode<<" modifiers:"<<e->modifiers()<<std::endl;
	if (e->button() == Qt::LeftButton) {
		_button_is_pressed = true;

        int idx = getSelectedBox();
        if (idx != -1) {
            std::cout<<"selected index"<<idx<<std::endl;
            redrawBoundingBox(idx);
        }

        if (_operation_mode == DRAW_MODE) {
            if(FILL_IN_MODIFIER==e->modifiers()) {
                // FILL in Operation
                _fill(e);
                return;
            } else if (BBOX_MODIFIER==e->modifiers() && _cid != -1) {
                // Select BOX for modification
                //check if its within range
                for(int i =0; i< box_list.size();i++){
                    if(box_list[i].isWithinBoundingBox(getXYonImage(e))){
                        box_list[i].select();
                        drawMarkedBoundingBox(box_list[i]);
                        return;
                    }
                }
                return;
            } else {
                _drawFillCircle(e);
                return;
            }
        }
	} else if(e->button() == Qt::RightButton){
        
    }
}

void ImageCanvas::mouseMoveEvent(QMouseEvent * e) {
	_mouse_pos.setX(e->x());
	_mouse_pos.setY(e->y());
    cv::Point cur_pt = getXYonImage(e);
	if (_button_is_pressed) { 
        _drawFillCircle(e);
    }
	update();
}

void ImageCanvas::reset(int operation){
    start_x =-1;
    start_y =-1;
    _operation_mode = operation;
    redrawBoundingBox();
    update();
}

void ImageCanvas::mouseReleaseEvent(QMouseEvent * e) {
	if(e->button() == Qt::LeftButton) {
        std::cout<<"mouse released"<<getXYonImage(e)<<std::endl;
		_button_is_pressed = false;
        if(_operation_mode == DRAW_MODE){
            _op_manager->save_draw();
        }
        _ui->setStarAtNameOfTab(true);
		_ui->undo_action->setEnabled(true);
	}

	if (e->button() == Qt::RightButton) { // selection of label
		QColor color = _mask.id.pixel(_mouse_pos / _scale);
		const LabelInfo * label = _ui->id_labels[color.red()];

		if(label->item != NULL) {
			emit(_ui->list_label->currentItemChanged(label->item, NULL));
        }

		refresh();
	}

	if (e->button() == Qt::MiddleButton)
	{
		cv::Point p = getXYonImage(e);
        int x = p.x;
        int y = p.y;

		_mask.exchangeLabel(x, y, _ui->id_labels, _color);
		update();
	}
}

void ImageCanvas::setSizePen(int pen_size) {
	_pen_size = pen_size;
}

void ImageCanvas::_drawFillCircle(QMouseEvent * e) {
	cv::Point p = getXYonImage(e);
    int x = p.x;
    int y = p.y;

    if (_pen_size > 0) {
		_mask.drawFillCircle(x, y, _pen_size, getColorMask(false));
        _smart_mask.drawFillCircle(x, y, _pen_size, getColorMask(true));
	} else {
		_mask.drawPixel(x, y, getColorMask(false));
        _smart_mask.drawPixel(x, y, getColorMask(true));
	}  

	update();
}

ColorMask ImageCanvas::getColorMask(bool smart) {

    ColorMask mask_color = _color;
    ColorMask smart_color = mask_color;

    int idx = getSelectedBox();

    // If a box is selected, modify the drawing operation
    if (idx != -1) {
        QColor bbox_color = box_list[idx].getMaskColor();

        int id = _ui->id_labels.getIdFromR(bbox_color.red());

        mask_color.id = QColor(id, id, id);
        mask_color.color = _ui->id_labels[id]->color;

        smart_color.id = mask_color.id;
        smart_color.color = bbox_color;
    } else {
        smart_color = _mask.getSmartColorMask(mask_color, _instance_num);
    }

    if (smart) {
        return smart_color;
    } else {
        return mask_color;
    }
}

void ImageCanvas::_fill(QMouseEvent *e){
    cv::Point p = getXYonImage(e);
    int x = p.x;
    int y = p.y;
    _mask.fill(x, y, getColorMask(false),_ui->id_labels);
    _smart_mask.fill(x, y, getColorMask(true));
}

void ImageCanvas::_startMarkingBoundingBox(QMouseEvent *e){
    cv::Point p = getXYonImage(e);
    this->start_x = p.x;
    this->start_y = p.y; 
    //store display to buffer
    _buffer_image = _image.copy();    
}

void ImageCanvas::_drawBoundingBox(QMouseEvent *e){
    //reset buffer to display on move
    _image = _buffer_image.copy();
    cv::Mat _image_mat = qImage2Mat(_image);
    rectangle(_image_mat,cv::Point(start_x,start_y),getXYonImage(e),cv::Scalar(255,0,0),2);
    _image = mat2QImage(_image_mat);
}

//from scratch
void ImageCanvas::redrawBoundingBox(int except_index){
    _image = _orig_image.copy();
    cv::Mat _image_mat = qImage2Mat(_image);
    for(int i =0; i < this->box_list.size(); i++){
        if(i == except_index){
            continue;
        }
        box_list[i].draw(_image_mat);//
    }
    _image = mat2QImage(_image_mat);
}

void ImageCanvas::clearMask() {
	_mask = ImageMask(_image.size());
    _smart_mask = ImageMask(_image.size());
	repaint();
}

void ImageCanvas::wheelEvent(QWheelEvent * event) {
	int delta = event->delta() > 0 ? 1 : -1;
	if (Qt::ShiftModifier == event->modifiers()) {
        _scroll_parent->verticalScrollBar()->setEnabled(false);
		int value = _ui->spinbox_pen_size->value() + delta * _ui->spinbox_pen_size->singleStep();
		_ui->spinbox_pen_size->setValue(value);
		emit(_ui->spinbox_pen_size->valueChanged(value));
		setSizePen(value);
		repaint();
	} else if (Qt::ControlModifier == event->modifiers()) {
		_scroll_parent->verticalScrollBar()->setEnabled(false);
		double value = _ui->spinbox_scale->value() + delta * _ui->spinbox_scale->singleStep();
		value = std::min<double>(_ui->spinbox_scale->maximum(),value);
		value = std::max<double>(_ui->spinbox_scale->minimum(), value);

		_ui->spinbox_scale->setValue(value);
		scaleChanged(value);
		repaint();
	} else {
        _scroll_parent->verticalScrollBar()->setEnabled(true);
	}
}

void ImageCanvas::keyPressEvent(QKeyEvent * event) {
	if (event->key() == Qt::Key_Space) {

        //emit(_ui->button_smart_mask->released());
	} else if(event->key() == Qt::Key_Delete){
        if(_operation_mode == BOX_SELECTED){
            _operation_mode = DRAW_MODE;
            int i = getSelectedBox();
            if(i== -1){
                return;
            }
            _op_manager->delete_bbox(box_list[i]);
            std::cout<<"box: ";
            box_list[i].printBoxParam();
            box_list.erase (box_list.begin()+i);
            redrawBoundingBox();
            update(); 
        }
    }
}


void ImageCanvas::setImageMask(const ImageMask & mask) {
	_mask = mask;
}


void ImageCanvas::setSmartImageMask(const ImageMask & smart_mask) {
    _smart_mask = smart_mask;
}

void ImageCanvas::setId(int id) {
	_color.id = QColor(id, id, id);
    _cid = id;
	_color.color = _ui->id_labels[id]->color;
}

void ImageCanvas::refresh() {
	update();
}


void ImageCanvas::undo() {
    if (_operation_mode == DRAW_MODE) {
        _op_manager->undo();
    }
    update();

	_ui->redo_action->setEnabled(true);
}

void ImageCanvas::redo() {
    if (_operation_mode == DRAW_MODE) {
        _op_manager->redo();
    }
    update();
	_ui->undo_action->setEnabled(true);
}

std::string ImageCanvas::getObjectString(){
    return _ui->id_labels[_cid]->name.toStdString();
}
