#pragma once
#include <map>
#include <set>
#include <unordered_map>
#include <QString>
#include "libdxfrw/libdxfrw.h"
#include "base/Graph.hpp"


class Bulge
{
public:
    Geo::Point start, end;
    double tangent = 0;

public:
    Bulge(const Geo::Point &s, const Geo::Point &e, const double t);

    Bulge(const double x0, const double y0, const double x1, const double y1, const double t);

    double radius() const;
	
    Geo::Point relative_center() const;

    Geo::Point center() const;

    double start_angle() const;

    double end_angle() const;

    bool is_cw() const;

    bool is_line() const;
};


class DXFReaderWriter : public DRW_Interface
{
private:
    bool _to_graph = true;
    bool _ignore_entity = false;
    int _version;
    Graph *_graph;
    Combination *_combination = nullptr;
    const ContainerGroup *_current_group = nullptr;
    dxfRW *_dxfrw = nullptr;

    std::set<std::string> _inserted_blocks;
    std::unordered_map<const Geo::Geometry *, int> _object_map;
    std::unordered_map<int, Geo::Geometry *> _handle_map;
    std::unordered_map<Combination *, int> _block_map;
    std::unordered_map<Combination *, std::string> _block_names;
    std::unordered_map<std::string, Combination *> _block_name_map;
    std::unordered_map<int, int> _handle_pairs; // handle-parentHandle

public:
    DXFReaderWriter(Graph *graph);

    DXFReaderWriter(Graph *graph, dxfRW *dxfrw);

    ~DXFReaderWriter();

    /** Called when header is parsed.  */
    void addHeader(const DRW_Header *data) override;

    /** Called for every line Type.  */
    void addLType(const DRW_LType &data) override;

    /** Called for every layer. */
    void addLayer(const DRW_Layer &data) override;

    /** Called for every dim style. */
    void addDimStyle(const DRW_Dimstyle &data) override;

    /** Called for every VPORT table. */
    void addVport(const DRW_Vport &data) override;

    /** Called for every text style. */
    void addTextStyle(const DRW_Textstyle &data) override;

    /** Called for every AppId entry. */
    void addAppId(const DRW_AppId &data) override;

    /* Called for every block. Note: all entities added after this
     * command go into this block until endBlock() is called. */
    void addBlock(const DRW_Block &data) override;

    /**
     * In DWG called when the following entities corresponding to a
     * block different from the current. Note: all entities added after this
     * command go into this block until setBlock() is called already.
     *
     * int handle are the value of DRW_Block::handleBlock added with addBlock()
     */
    void setBlock(const int handle) override;

    /** Called to end the current block */
    void endBlock() override;

    /** Called for every point */
    void addPoint(const DRW_Point &data) override;

    /** Called for every line */
    void addLine(const DRW_Line &data) override;

    /** Called for every ray */
    void addRay(const DRW_Ray &data) override;

    /** Called for every xline */
    void addXline(const DRW_Xline &data) override;

    /** Called for every arc */
    void addArc(const DRW_Arc &data) override;

    /** Called for every circle */
    void addCircle(const DRW_Circle &data) override;

    /** Called for every ellipse */
    void addEllipse(const DRW_Ellipse &data) override;

    /** Called for every lwpolyline */
    void addLWPolyline(const DRW_LWPolyline &data) override;

    /** Called for every polyline start */
    void addPolyline(const DRW_Polyline &data) override;

    /** Called for every spline */
    void addSpline(const DRW_Spline* data) override;
	
	/** Called for every spline knot value */
    void addKnot(const DRW_Entity &data) override;

    /** Called for every insert. */
    void addInsert(const DRW_Insert &data) override;
    
    /** Called for every trace start */
    void addTrace(const DRW_Trace &data) override;
    
    /** Called for every 3dface start */
    void add3dFace(const DRW_3Dface &data) override;

    /** Called for every solid start */
    void addSolid(const DRW_Solid &data) override;


    /** Called for every Multi Text entity. */
    void addMText(const DRW_MText &data) override;

    /** Called for every Text entity. */
    void addText(const DRW_Text &data) override;

    /**
     * Called for every aligned dimension entity. 
     */
    void addDimAlign(const DRW_DimAligned *data) override;

    /**
     * Called for every linear or rotated dimension entity. 
     */
    void addDimLinear(const DRW_DimLinear *data) override;

	/**
     * Called for every radial dimension entity. 
     */
    void addDimRadial(const DRW_DimRadial *data) override;

	/**
     * Called for every diametric dimension entity. 
     */
    void addDimDiametric(const DRW_DimDiametric *data) override;

	/**
     * Called for every angular dimension (2 lines version) entity. 
     */
    void addDimAngular(const DRW_DimAngular *data) override;

	/**
     * Called for every angular dimension (3 points version) entity. 
     */
    void addDimAngular3P(const DRW_DimAngular3p *data) override;
	
    /**
     * Called for every ordinate dimension entity. 
     */
    void addDimOrdinate(const DRW_DimOrdinate *data) override;
    
    /** 
	 * Called for every leader start. 
	 */
    void addLeader(const DRW_Leader *data) override;
	
	/** 
	 * Called for every hatch entity. 
	 */
    void addHatch(const DRW_Hatch *data) override;
	
    /**
     * Called for every viewport entity.
     */
    void addViewport(const DRW_Viewport &data) override;

    /**
	 * Called for every image entity. 
	 */
    void addImage(const DRW_Image *data) override;

	/**
	 * Called for every image definition.
	 */
    void linkImage(const DRW_ImageDef *data) override;

    /**
     * Called for every comment in the DXF file (code 999).
     */
    void addComment(const char *comment) override;


    void writeHeader(DRW_Header &data) override;

    void writeBlocks() override;

    void writeBlockRecords() override;

    void writeEntities() override;

    void writeLTypes() override;

    void writeLayers() override;

    void writeTextstyles() override;

    void writeVports() override;

    void writeDimstyles() override;

    void writeAppId() override;

private:
    void write_geometry_object(const Geo::Geometry *object);

    void write_bezier(const Geo::Bezier *bezier);

    void write_bspline(const Geo::BSpline *bspline);

    void write_circle(const Geo::Circle *circle);

    void write_ellipse(const Geo::Ellipse *ellipse);

    void write_line(const Geo::Line *line);

    void write_polygon(const Geo::Polygon *polygon);

    void write_polyline(const Geo::Polyline *polyline);

    void write_text(const Text *text);

    void write_arc(const Geo::Arc *arc);

public:
    void check_block();

    void clear_empty_group();

    static QString to_dxf_string(const QString &txt);

    static QString to_native_string(const QString &txt);

};