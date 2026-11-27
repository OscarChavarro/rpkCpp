package vsdk.toolkit.environment.geometry.surface.polygon;

import java.io.Serial;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import vsdk.toolkit.environment.geometry.element.Vertex2D;
import vsdk.toolkit.environment.geometry.element.Ray;
import vsdk.toolkit.common.dataStructures.BinaryTreeNode;
import vsdk.toolkit.environment.geometry.element.RayHit;
import vsdk.toolkit.environment.geometry.surface.Surface;

public class Polygon2D extends Surface
{
    @Serial
    private static final long serialVersionUID = 20260419L;

    public List<_Polygon2DContour> loops;
    private _Polygon2DContour currentLoop;
    // headNode is the head node of the n-ary tree codified in a binary tree
    // in left child-right sibling way.
    private BinaryTreeNode<_Polygon2DContour> headNode;
    
    public Polygon2D()
    {
        loops = new ArrayList<_Polygon2DContour>();
        nextLoop();
    }

    public void addVertex(double x, double y, double r, double g, double b)
    {
        currentLoop.addVertex(x, y, r, g, b);
    }

    public void addVertex(double x, double y)
    {
        currentLoop.addVertex(x, y);
    }

    public void pushVertex(double x, double y)
    {
        currentLoop.pushVertex(x, y);
    }

    public final void nextLoop()
    {
        currentLoop = new _Polygon2DContour();
        loops.add(currentLoop);
    }

    public final void eraseLastLoop()
    {
        int i;
        _Polygon2DContour loop;
        
        if(loops.size() > 1) {
            // Remove also the holes of that loop. LRR.
            for(i=0; i<loops.size(); ++i) {
                loop = loops.get(i);
                if(loop.getExteriorContour() == loops.get(loops.size()-1)) {
                    loops.remove(i);
                    --i;
                }
            }
            if(loops.size() > 1) {
                loops.remove(loops.size()-1);
                currentLoop = loops.get(loops.size()-1);
            }
            // Rebuild the n-ary tree codified in a binary tree.
            classifyContourHoles();
        }
    }
    
    public void invert()
    {
        for(_Polygon2DContour p2DContour : loops) {
            Collections.reverse(p2DContour.vertices);
        }
    }
    
    /**
    @return a new 6 valued double array containing the coordinates of a min-max
    bounding box for current geometry.
    */
    @Override
    public double[] getMinMax()
    {
        double minMax[];
        _Polygon2DContour l;
        Vertex2D v;
        int i;
        int j;
        minMax = new double[6];

        double minX = Double.MAX_VALUE;
        double minY = Double.MAX_VALUE;
        double maxX = -Double.MAX_VALUE;
        double maxY = -Double.MAX_VALUE;

        for ( i = 0; i < loops.size(); i++ ) {
            for ( j = 0; j < loops.get(i).vertices.size() - 1; j++ ) {
                v = loops.get(i).vertices.get(j);
                if ( v.x > maxX ) {
                    maxX = v.x;
                }
                if ( v.x < minX ) {
                    minX = v.x;
                }
                if ( v.y > maxY ) {
                    maxY = v.y;
                }
                if ( v.y < minY ) {
                    minY = v.y;
                }
            }
        }

        minMax[0] = minX;
        minMax[1] = minY;
        minMax[2] = 0;
        minMax[3] = maxX;
        minMax[4] = maxY;
        minMax[5] = 0;

        return minMax;
    }

    public Ray doIntersectionFirstHit(Ray inOut_ray)
    {
        return null;
    }

    @Override
    public boolean doIntersectionFirstHit(Ray inRay, RayHit outHit)
    {
        return false;
    }
    public void
    doExtraInformation(Ray inRay, double intT, 
                       RayHit outData)
    {

    }
    
    /**
     * @return the headNode
     */
    public BinaryTreeNode<_Polygon2DContour> getHeadNode() {
        return headNode;
    }

    /**
     * @param headNode the headNode to set
     */
    public void setHeadNode(BinaryTreeNode<_Polygon2DContour> headNode) {
        this.headNode = headNode;
    }
    // Contour classification, moved here from the removed PolygonProcessor as
    // cpp did (Polygon2D::classifyContourHoles / isPointInsideContour). The
    // Java bodies are the original algorithm (n-ary contour tree, EPSILON
    // boundary band) that the cpp version simplifies; they are kept as they were.
    private static final double EPSILON = 1E-18;

    public void classifyContourHoles()
    {
        Polygon2D polygon = this;
        int i,j;
        //Node of the Great Hole that contains everything.
        ArrayList<_Polygon2DContour> tempList;
        _Polygon2DContour p2DContour,p2DContourTest;

        // Clear temporary tree flags.
        for(i=0; i<polygon.loops.size(); ++i) {
            p2DContour = polygon.loops.get(i);
            // Indicates that the contour is in the binary tree.
            p2DContour.fleetingFlag = false;
        }
        // Head node contains no data.
        polygon.setHeadNode(new BinaryTreeNode<_Polygon2DContour>(null));
        tempList = new ArrayList<_Polygon2DContour>();
        // Fill tempList and create the binary tree.
        for(i=0; i<polygon.loops.size(); ++i) {
            p2DContour = polygon.loops.get(i);
            tempList.clear();
            tempList.add(p2DContour);
            for(j=i+1; j<polygon.loops.size(); ++j) {
                p2DContourTest = polygon.loops.get(j);
                if(!p2DContourTest.fleetingFlag) {
                     // If p2DContourTest is not in the binary tree:
                    if(isContourInsideContour(p2DContour.vertices
                            ,p2DContourTest.vertices)) {
                        tempList.add(p2DContourTest);
                    }
                }
            }
            // Sorts in descending order.
            Collections.sort(tempList, Collections.reverseOrder());
            insertListInBinaryTree(tempList,polygon.getHeadNode());
        }
    }

    private static void insertListInBinaryTree(
             ArrayList<_Polygon2DContour> list
            ,BinaryTreeNode<_Polygon2DContour> headNode) // Head node is in the 0 level.
    {
        _Polygon2DContour p2DContour;
        BinaryTreeNode<_Polygon2DContour> containingContourNode;
        int levelOfContainingContourNode[] = new int[1];

        if(list.isEmpty())
            return;
        p2DContour = list.get(0);
        containingContourNode = findContainingContourNode(
                 p2DContour
                ,headNode
                ,levelOfContainingContourNode);
        insertListInBinaryTreeNode(list,containingContourNode,levelOfContainingContourNode[0]);
    }

    private static void insertListInBinaryTreeNode(
             ArrayList<_Polygon2DContour> list
            ,BinaryTreeNode<_Polygon2DContour> containingListNode
            ,int levelOfContainingContourNode)
    {
        int i;
        BinaryTreeNode<_Polygon2DContour> child,lastChild=null;
        _Polygon2DContour p2DContour;
        int levelOfNode;

        levelOfNode = levelOfContainingContourNode + 1;
        if((levelOfNode%2) == 0)
            list.get(0).setExteriorContour(containingListNode.getData());
        // Find the last child of the node and add a new one.
        child = containingListNode.getChild();
        if(child == null) {
            lastChild = new BinaryTreeNode<_Polygon2DContour>(list.get(0));
            containingListNode.setChild(lastChild);
        } else {
            while(child != null) {
                lastChild = child;
                child = child.getSibling();
            }
            child = new BinaryTreeNode<_Polygon2DContour>(list.get(0));
            if ( lastChild != null ) {
                lastChild.setSibling(child);
            }
            lastChild = child;
        }
        // Indicates that the contour is in the binary tree.
        list.get(0).fleetingFlag = true;
        // Fill the binary tree with the rest of elements of list.
        for(i=1; i<list.size(); ++i) {
            ++levelOfNode;
            p2DContour = list.get(i);
            if((levelOfNode%2) == 0) { // Is a hole.
                p2DContour.setExteriorContour(lastChild.getData());
            }
            child = new BinaryTreeNode<_Polygon2DContour>(p2DContour);
            lastChild.setChild(child);
            child.getData().fleetingFlag = true;
            lastChild = child;
        }
    }

    /**
     * Finds the contour node that contains the p2DContour in the binary tree
     * represented by headNode.
     * @param p2DContour
     * @param headNode
     * @param levelOfContainingContourNode is zero based. This array contains one element.
     * @return
     */
    private static BinaryTreeNode<_Polygon2DContour> findContainingContourNode(
             _Polygon2DContour p2DContour
            ,BinaryTreeNode<_Polygon2DContour> headNode
            ,int levelOfContainingContourNode[])
    {
        boolean isTheContainingContourNode;
        BinaryTreeNode<_Polygon2DContour> child;

        levelOfContainingContourNode[0] = 0;
        while(headNode != null) {
            child = headNode.getChild();
            isTheContainingContourNode = true;
            while(child != null) {
                if(isContourInsideContour(p2DContour.vertices,child.getData().vertices)) {
                    isTheContainingContourNode = false;
                    //findContainingContourNode(p2DContour,child);
                    headNode = child;
                    ++levelOfContainingContourNode[0];
                    break;
                }
                child = child.getSibling();
            }
            if(isTheContainingContourNode)
                return headNode;
        }
        // Reaches here only if headNode == null.
        return headNode;
    }

    @SuppressWarnings("unused")
    private static boolean binaryTreeContainContour(
             BinaryTreeNode<_Polygon2DContour> headNode
            ,_Polygon2DContour p2DContour)
    {
        if(headNode == null)
            return false;
        if(headNode.getData() == p2DContour)
            return true;
        if(binaryTreeContainContour(headNode.getSibling(),p2DContour))
            return true;
        return binaryTreeContainContour(headNode.getChild(),p2DContour);
    }

    /**
     * Test if the contour is inside the polygon. This function assumes that
     * the contour is fully inside or fully outside.
     * @param contour
     * @param mainPolygon
     * @return
     */
    private static boolean isContourInsideContour(
             List<Vertex2D> contour
            ,List<Vertex2D> mainPolygon)
    {
        int i;
        Vertex2D point;
        byte result;

        for (i = 0; i < contour.size(); ++i) {
            point = contour.get(i);

            result = isPointInsideContour(point, mainPolygon);
            if(result == 1)
                return true;
            if(result == -1)
                return false;
            // If result equals 0 the point is in the contour of the mainPolygon,
            // the search continues.
        }
        // If every point is in the contour of the mainPolygon. Note that in this
        // case the contour can be anywhere(inside, outside, intersecting).
        return false;
    }


    /* Function isPointInPolygon2D is based on the function developed by W. Randolph
     Franklin:
     http://www.ecse.rpi.edu/Homepages/wrf/Research/Short_Notes/pnpoly.html
     visited on sep 08 2014.
     The part that tests if the point is in de boundary is new.

     License to Use

     Copyright (c) 1970-2003, Wm. Randolph Franklin

     Permission is hereby granted, free of charge, to any person obtaining a
     copy of this software and associated documentation files (the "Software"),
     to deal in the Software without restriction, including without limitation
     the rights to use, copy, modify, merge, publish, distribute, sublicense,
     and/or sell copies of the Software, and to permit persons to whom the
     Software is furnished to do so, subject to the following conditions:

     Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimers.
     Redistributions in binary form must reproduce the above copyright notice
     in the documentation and/or other materials provided with the distribution.
     The name of W. Randolph Franklin may not be used to endorse or promote
     products derived from this Software without specific prior written permission.
     THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
     OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
     FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
     AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
     WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
     IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
     */
    /**
     * Test if point is inside, outside or on the boundary of the polygon.
     *
     * @param point
     * @param polygon
     * @return 1 if the point is inside, -1 if the point is outside, 0 if the
     * point is on the boundary.
     */
    public static byte isPointInsideContour(Vertex2D point,
                                              List<Vertex2D> polygon)
    {
        int i,polSize;
        boolean isInside = false;
        double temp;
        Vertex2D polPoint,polPointNext;

        polSize = polygon.size();
        if(polSize<3) {
            return -1;
        }
        for(i=0; i<polSize; ++i) {
//            if(point.y == polPoint.y && point.x<polPoint.x){
//                if(polPoint.y<polPointNext.y != polPoint.y<dllnNode.previous.data.y)
//                    isInside = !isInside;
//            } else
            polPoint = polygon.get(i);
            if(i == polSize-1)
                polPointNext = polygon.get(0);
            else
                polPointNext = polygon.get(i+1);
            temp = polPoint.y - polPointNext.y;
            if ( Math.abs(temp) < EPSILON ) { //Horizontal line.
                if ( Math.abs(point.y - (polPointNext.y + temp / 2)) < EPSILON ) {
                    if ( (polPointNext.x - point.x) * (point.x - polPoint.x) >= 0 ) {
                        return 0;
                    }
                }
            }
            if ( point.y < polPoint.y != point.y < polPointNext.y ) {
                temp = ((polPointNext.x - polPoint.x) * (point.y - polPoint.y)
                        / (polPointNext.y - polPoint.y) + polPoint.x);
                if ( Math.abs(point.x - temp) < EPSILON ) {
                    return 0;
                }
                if ( point.x < temp ) {
                    isInside = !isInside;
                }
            }
        }
        if ( isInside ) {
            return 1;
        } else {
            return -1;
        }
    }
}
