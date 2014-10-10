var NAVTREE =
[
  [ "LCD Library", "index.html", [
    [ "Class List", "annotated.html", [
      [ "I2CIO", "class_i2_c_i_o.html", null ],
      [ "LCD", "class_l_c_d.html", null ],
      [ "LiquidCrystal", "class_liquid_crystal.html", null ],
      [ "LiquidCrystal_I2C", "class_liquid_crystal___i2_c.html", null ],
      [ "LiquidCrystal_I2C_ByVac", "class_liquid_crystal___i2_c___by_vac.html", null ],
      [ "LiquidCrystal_SR", "class_liquid_crystal___s_r.html", null ],
      [ "LiquidCrystal_SR1W", "class_liquid_crystal___s_r1_w.html", null ],
      [ "LiquidCrystal_SR2W", "class_liquid_crystal___s_r2_w.html", null ],
      [ "LiquidCrystal_SR3W", "class_liquid_crystal___s_r3_w.html", null ]
    ] ],
    [ "Class Index", "classes.html", null ],
    [ "Class Hierarchy", "hierarchy.html", [
      [ "I2CIO", "class_i2_c_i_o.html", null ],
      [ "LCD", "class_l_c_d.html", [
        [ "LiquidCrystal", "class_liquid_crystal.html", null ],
        [ "LiquidCrystal_I2C", "class_liquid_crystal___i2_c.html", null ],
        [ "LiquidCrystal_I2C_ByVac", "class_liquid_crystal___i2_c___by_vac.html", null ],
        [ "LiquidCrystal_SR", "class_liquid_crystal___s_r.html", null ],
        [ "LiquidCrystal_SR1W", "class_liquid_crystal___s_r1_w.html", null ],
        [ "LiquidCrystal_SR2W", "class_liquid_crystal___s_r2_w.html", null ],
        [ "LiquidCrystal_SR3W", "class_liquid_crystal___s_r3_w.html", null ]
      ] ]
    ] ],
    [ "Class Members", "functions.html", null ],
    [ "File List", "files.html", [
      [ "FastIO.cpp", "_fast_i_o_8cpp.html", null ],
      [ "FastIO.h", "_fast_i_o_8h.html", null ],
      [ "I2CIO.cpp", "_i2_c_i_o_8cpp.html", null ],
      [ "I2CIO.h", "_i2_c_i_o_8h.html", null ],
      [ "LCD.cpp", "_l_c_d_8cpp.html", null ],
      [ "LCD.h", "_l_c_d_8h.html", null ],
      [ "LiquidCrystal.cpp", "_liquid_crystal_8cpp.html", null ],
      [ "LiquidCrystal.h", "_liquid_crystal_8h.html", null ],
      [ "LiquidCrystal_I2C.cpp", "_liquid_crystal___i2_c_8cpp.html", null ],
      [ "LiquidCrystal_I2C.h", "_liquid_crystal___i2_c_8h.html", null ],
      [ "LiquidCrystal_I2C_ByVac.cpp", "_liquid_crystal___i2_c___by_vac_8cpp.html", null ],
      [ "LiquidCrystal_I2C_ByVac.h", "_liquid_crystal___i2_c___by_vac_8h.html", null ],
      [ "LiquidCrystal_SR.cpp", "_liquid_crystal___s_r_8cpp.html", null ],
      [ "LiquidCrystal_SR.h", "_liquid_crystal___s_r_8h.html", null ],
      [ "LiquidCrystal_SR1W.cpp", "_liquid_crystal___s_r1_w_8cpp.html", null ],
      [ "LiquidCrystal_SR1W.h", "_liquid_crystal___s_r1_w_8h.html", null ],
      [ "LiquidCrystal_SR2W.cpp", "_liquid_crystal___s_r2_w_8cpp.html", null ],
      [ "LiquidCrystal_SR2W.h", "_liquid_crystal___s_r2_w_8h.html", null ],
      [ "LiquidCrystal_SR3W.cpp", "_liquid_crystal___s_r3_w_8cpp.html", null ],
      [ "LiquidCrystal_SR3W.h", "_liquid_crystal___s_r3_w_8h.html", null ]
    ] ],
    [ "File Members", "globals.html", null ]
  ] ]
];

function createIndent(o,domNode,node,level)
{
  if (node.parentNode && node.parentNode.parentNode)
  {
    createIndent(o,domNode,node.parentNode,level+1);
  }
  var imgNode = document.createElement("img");
  if (level==0 && node.childrenData)
  {
    node.plus_img = imgNode;
    node.expandToggle = document.createElement("a");
    node.expandToggle.href = "javascript:void(0)";
    node.expandToggle.onclick = function() 
    {
      if (node.expanded) 
      {
        $(node.getChildrenUL()).slideUp("fast");
        if (node.isLast)
        {
          node.plus_img.src = node.relpath+"ftv2plastnode.png";
        }
        else
        {
          node.plus_img.src = node.relpath+"ftv2pnode.png";
        }
        node.expanded = false;
      } 
      else 
      {
        expandNode(o, node, false);
      }
    }
    node.expandToggle.appendChild(imgNode);
    domNode.appendChild(node.expandToggle);
  }
  else
  {
    domNode.appendChild(imgNode);
  }
  if (level==0)
  {
    if (node.isLast)
    {
      if (node.childrenData)
      {
        imgNode.src = node.relpath+"ftv2plastnode.png";
      }
      else
      {
        imgNode.src = node.relpath+"ftv2lastnode.png";
        domNode.appendChild(imgNode);
      }
    }
    else
    {
      if (node.childrenData)
      {
        imgNode.src = node.relpath+"ftv2pnode.png";
      }
      else
      {
        imgNode.src = node.relpath+"ftv2node.png";
        domNode.appendChild(imgNode);
      }
    }
  }
  else
  {
    if (node.isLast)
    {
      imgNode.src = node.relpath+"ftv2blank.png";
    }
    else
    {
      imgNode.src = node.relpath+"ftv2vertline.png";
    }
  }
  imgNode.border = "0";
}

function newNode(o, po, text, link, childrenData, lastNode)
{
  var node = new Object();
  node.children = Array();
  node.childrenData = childrenData;
  node.depth = po.depth + 1;
  node.relpath = po.relpath;
  node.isLast = lastNode;

  node.li = document.createElement("li");
  po.getChildrenUL().appendChild(node.li);
  node.parentNode = po;

  node.itemDiv = document.createElement("div");
  node.itemDiv.className = "item";

  node.labelSpan = document.createElement("span");
  node.labelSpan.className = "label";

  createIndent(o,node.itemDiv,node,0);
  node.itemDiv.appendChild(node.labelSpan);
  node.li.appendChild(node.itemDiv);

  var a = document.createElement("a");
  node.labelSpan.appendChild(a);
  node.label = document.createTextNode(text);
  a.appendChild(node.label);
  if (link) 
  {
    a.href = node.relpath+link;
  } 
  else 
  {
    if (childrenData != null) 
    {
      a.className = "nolink";
      a.href = "javascript:void(0)";
      a.onclick = node.expandToggle.onclick;
      node.expanded = false;
    }
  }

  node.childrenUL = null;
  node.getChildrenUL = function() 
  {
    if (!node.childrenUL) 
    {
      node.childrenUL = document.createElement("ul");
      node.childrenUL.className = "children_ul";
      node.childrenUL.style.display = "none";
      node.li.appendChild(node.childrenUL);
    }
    return node.childrenUL;
  };

  return node;
}

function showRoot()
{
  var headerHeight = $("#top").height();
  var footerHeight = $("#nav-path").height();
  var windowHeight = $(window).height() - headerHeight - footerHeight;
  navtree.scrollTo('#selected',0,{offset:-windowHeight/2});
}

function expandNode(o, node, imm)
{
  if (node.childrenData && !node.expanded) 
  {
    if (!node.childrenVisited) 
    {
      getNode(o, node);
    }
    if (imm)
    {
      $(node.getChildrenUL()).show();
    } 
    else 
    {
      $(node.getChildrenUL()).slideDown("fast",showRoot);
    }
    if (node.isLast)
    {
      node.plus_img.src = node.relpath+"ftv2mlastnode.png";
    }
    else
    {
      node.plus_img.src = node.relpath+"ftv2mnode.png";
    }
    node.expanded = true;
  }
}

function getNode(o, po)
{
  po.childrenVisited = true;
  var l = po.childrenData.length-1;
  for (var i in po.childrenData) 
  {
    var nodeData = po.childrenData[i];
    po.children[i] = newNode(o, po, nodeData[0], nodeData[1], nodeData[2],
        i==l);
  }
}

function findNavTreePage(url, data)
{
  var nodes = data;
  var result = null;
  for (var i in nodes) 
  {
    var d = nodes[i];
    if (d[1] == url) 
    {
      return new Array(i);
    }
    else if (d[2] != null) // array of children
    {
      result = findNavTreePage(url, d[2]);
      if (result != null) 
      {
        return (new Array(i).concat(result));
      }
    }
  }
  return null;
}

function initNavTree(toroot,relpath)
{
  var o = new Object();
  o.toroot = toroot;
  o.node = new Object();
  o.node.li = document.getElementById("nav-tree-contents");
  o.node.childrenData = NAVTREE;
  o.node.children = new Array();
  o.node.childrenUL = document.createElement("ul");
  o.node.getChildrenUL = function() { return o.node.childrenUL; };
  o.node.li.appendChild(o.node.childrenUL);
  o.node.depth = 0;
  o.node.relpath = relpath;

  getNode(o, o.node);

  o.breadcrumbs = findNavTreePage(toroot, NAVTREE);
  if (o.breadcrumbs == null)
  {
    o.breadcrumbs = findNavTreePage("index.html",NAVTREE);
  }
  if (o.breadcrumbs != null && o.breadcrumbs.length>0)
  {
    var p = o.node;
    for (var i in o.breadcrumbs) 
    {
      var j = o.breadcrumbs[i];
      p = p.children[j];
      expandNode(o,p,true);
    }
    p.itemDiv.className = p.itemDiv.className + " selected";
    p.itemDiv.id = "selected";
    $(window).load(showRoot);
  }
}

