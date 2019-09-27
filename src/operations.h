#ifndef OPERATIONS_H
#define OPERATIONS_H

#include "image_mask.h"
#include "image_canvas.h"

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <QImage>

class ImageCanvas;

class Operations {
public:
	Operations() {}
	~Operations() {}
	Operations(ImageCanvas * canvas) : _canvas(canvas) {}
	virtual void undo() {};
	virtual void redo() = 0;
	virtual void speak() { std::cout << "Operations" << std::endl; }

protected:
	ImageCanvas *_canvas;
};

class DrawOperation : public Operations {
public:
	DrawOperation() {}
	~DrawOperation() {}
	DrawOperation(ImageCanvas * canvas, MaskDiff mask_diff, MaskDiff smart_mask_diff) : Operations(canvas), _mask_diff(mask_diff), _smart_mask_diff(smart_mask_diff) {}
	virtual void undo();
	void redo();
	virtual void speak() { std::cout << "Draw" << std::endl; }

protected:
	MaskDiff _mask_diff;
	MaskDiff _smart_mask_diff;
};

class BoundingBoxOperation : public Operations {
public:
	BoundingBoxOperation() {}
	~BoundingBoxOperation() {}
	BoundingBoxOperation(ImageCanvas * canvas, BoundingBox bbox) : Operations(canvas), _new_bbox(bbox) {}
	virtual void undo() = 0;
	virtual void redo() = 0;
	virtual void speak() { std::cout << "BBOX OP" << std::endl; }

protected:
	BoundingBox _new_bbox;
};

class CreateBoxOperation : public BoundingBoxOperation {
public:
	CreateBoxOperation() {}
	~CreateBoxOperation() {}
	CreateBoxOperation(ImageCanvas * canvas, BoundingBox bbox, bool create_flag) : BoundingBoxOperation(canvas, bbox), _create_flag(create_flag) {}
	void undo();
	void redo();
	virtual void speak() { std::cout << "Create BBOX" << std::endl; }

protected:
	bool _create_flag;
};

class ChangeBoxOperation : public BoundingBoxOperation {
public:
	ChangeBoxOperation() {}	
	~ChangeBoxOperation() {}
	ChangeBoxOperation(ImageCanvas * canvas, BoundingBox new_bbox, BoundingBox old_bbox, int index) : 
	BoundingBoxOperation(canvas, new_bbox), _old_bbox(old_bbox), _index(index) {}

	void undo();
	void redo();
	virtual void speak() { std::cout << "Change BBOX" << std::endl; }
protected:
	int _index;
	BoundingBox _old_bbox;
};

class OperationManager {
public:
	OperationManager() {}
	~OperationManager() {}
	OperationManager(ImageCanvas * canvas);
	void save_draw();
	void create_bbox(BoundingBox bbox);
	void delete_bbox(BoundingBox bbox);
	void change_bbox(BoundingBox bbox, int index);

	void undo();
	void redo();

protected:
	ImageCanvas *_canvas;

	Mask curr_mask;
	Mask curr_smart_mask;
	std::vector<BoundingBox> curr_bbox_list;

	std::vector<Operations*> op_list;
	int _index = 0;

	void add_op(Operations * op);
	void sync();
};

#endif