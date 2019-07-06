#include "config.h"


void dao_oal_test(void)
{

	FILE *fp;
    mxml_node_t *xml;    /* <?xml ... ?> */
    mxml_node_t *data;   /* <data> */
    mxml_node_t *node;   /* <node> */
    mxml_node_t *group;  /* <group> */

    xml = mxmlNewXML("1.0");

    data = mxmlNewElement(xml, "data");

    node = mxmlNewElement(data, "node");
    mxmlNewText(node, 0, "val1");
    node = mxmlNewElement(data, "node");
    mxmlNewText(node, 0, "val2");
    node = mxmlNewElement(data, "node");
    mxmlNewText(node, 0, "val3");

    group = mxmlNewElement(data, "group");

    node = mxmlNewElement(group, "node");
    mxmlNewText(node, 0, "val4");
    node = mxmlNewElement(group, "node");
    mxmlNewText(node, 0, "val5");
    node = mxmlNewElement(group, "node");
    mxmlNewText(node, 0, "val6");

    node = mxmlNewElement(data, "node");
    mxmlNewText(node, 0, "val7");
    node = mxmlNewElement(data, "node");
    mxmlNewText(node, 0, "val8");
	fp = fopen("filename1.xml", "w");
    mxmlSaveFile(xml, fp, MXML_NO_CALLBACK);
    fclose(fp);

}
