import paho.mqtt.client as mqtt

import time
import json
import threading
from gpsdclient import GPSDClient


orgResponse = {
    "data": {
        "listSignsQuery": {
            "signs": [
                {
                    "__typename": "Sign",
                    "uri": "electronic-sign/irisxsigns*78",
                    "title": "US 30: US 30 WB Mile 13.0",
                    "cityReference": "in Hobart",
                    "bbox": [
                        -87.2923,
                        41.47111,
                        -87.2923,
                        41.47111
                    ],
                    "icon": "/images/icon-sign-active-fill-solid.svg",
                    "color": "rgb(59,55,55)",
                    "signDisplayType": "CMS",
                    "signStatus": "DISPLAYING_MESSAGE",
                    "location": {
                        "primaryLinearReference": 12.35798534445847,
                        "secondaryLinearReference": 'null',
                        "routeDesignator": "US 30"
                    },
                    "views": [
                        {
                            "uri": "electronic-sign/irisxsigns*78/1190240041",
                            "title": "US 30: US 30 WB Mile 13.0",
                            "category": "CMS",
                            "__typename": "SignTextView",
                            "textJustification": "CENTER",
                            "textLines": [
                                "FWY TRAVEL TIME TO",
                                "I-94   7 MI    7 MIN",
                                "94/ILL 18 MI  19 MIN"
                            ],
                            "parentCollection": {
                                "uri": "electronic-sign/irisxsigns*78"
                            }
                        }
                    ]
                },
                {
                    "__typename": "Sign",
                    "uri": "electronic-sign/irisxsigns*79",
                    "title": "US 31: US 31 SB Mile 124.7",
                    "cityReference": "in Carmel",
                    "bbox": [
                        -86.158,
                        39.94931,
                        -86.158,
                        39.94931
                    ],
                    "icon": "/images/icon-sign-active-fill-solid.svg",
                    "color": "rgb(59,55,55)",
                    "signDisplayType": "CMS",
                    "signStatus": "DISPLAYING_MESSAGE",
                    "location": {
                        "primaryLinearReference": 108.58334884493193,
                        "secondaryLinearReference": 'null',
                        "routeDesignator": "US 31"
                    },
                    "views": [
                        {
                            "uri": "electronic-sign/irisxsigns*79/3523198824",
                            "title": "US 31: US 31 SB Mile 124.7",
                            "category": "CMS",
                            "__typename": "SignTextView",
                            "textJustification": "CENTER",
                            "textLines": [
                                "TRAVEL TIME TO",
                                "465W/865 7 MI  9 MIN",
                                "465E/69  8 MI  9 MIN"
                            ],
                            "parentCollection": {
                                "uri": "electronic-sign/irisxsigns*79"
                            }
                        }
                    ]
                },
                {
                    "__typename": "Sign",
                    "uri": "electronic-sign/indianasigns*1658",
                    "title": "US 31: US 31 NB Mile 126.0",
                    "cityReference": "in Carmel",
                    "bbox": [
                        -86.1575,
                        39.96806,
                        -86.1575,
                        39.96806
                    ],
                    "icon": "/images/icon-traveltime-active-fill-solid.svg",
                    "color": "rgb(59,55,55)",
                    "signDisplayType": "OVERLAY_TRAVEL_TIME",
                    "signStatus": "DISPLAYING_MESSAGE",
                    "location": {
                        "primaryLinearReference": 109.87637675581392,
                        "secondaryLinearReference": 'null',
                        "routeDesignator": "US 31"
                    },
                    "views": [
                        {
                            "uri": "electronic-sign/indianasigns*1658/4133249105",
                            "title": "US 31: US 31 NB Mile 126.0",
                            "category": "OVERLAY_TRAVEL_TIME",
                            "__typename": "SignOverlayView",
                            "travelTimes": [
                                "6",
                                "10"
                            ],
                            "imageUrl": "/images/overlay-signs/1658.png",
                            "imageLayout": "ROW2_A",
                            "parentCollection": {
                                "uri": "electronic-sign/indianasigns*1658"
                            }
                        }
                    ]
                },
                {
                    "__typename": "Sign",
                    "uri": "electronic-sign/irisxsigns*80",
                    "title": "US 31: US 31 NB Mile 126.2",
                    "cityReference": "in Carmel",
                    "bbox": [
                        -86.15763,
                        39.97093,
                        -86.15763,
                        39.97093
                    ],
                    "icon": "/images/icon-sign-active-fill-solid.svg",
                    "color": "rgb(59,55,55)",
                    "signDisplayType": "CMS",
                    "signStatus": "DISPLAYING_MESSAGE",
                    "location": {
                        "primaryLinearReference": 110.07403070970464,
                        "secondaryLinearReference": 'null',
                        "routeDesignator": "US 31"
                    },
                    "views": [
                        {
                            "uri": "electronic-sign/irisxsigns*80/2148353742",
                            "title": "US 31: US 31 NB Mile 126.2",
                            "category": "CMS",
                            "__typename": "SignTextView",
                            "textJustification": "CENTER",
                            "textLines": [
                                "161ST  5 MI   5 MIN",
                                "191ST  8 MI   8 MIN",
                                "236TH 12 MI  13 MIN"
                            ],
                            "parentCollection": {
                                "uri": "electronic-sign/irisxsigns*80"
                            }
                        }
                    ]
                },
                {
                    "__typename": "Sign",
                    "uri": "electronic-sign/irisxsigns*81",
                    "title": "US 31: US 31 SB Mile 131.2",
                    "cityReference": "in Westfield",
                    "bbox": [
                        -86.1343,
                        40.02857,
                        -86.1343,
                        40.02857
                    ],
                    "icon": "/images/icon-sign-active-fill-solid.svg",
                    "color": "rgb(59,55,55)",
                    "signDisplayType": "CMS",
                    "signStatus": "DISPLAYING_MESSAGE",
                    "location": {
                        "primaryLinearReference": 115.03436012814909,
                        "secondaryLinearReference": 'null',
                        "routeDesignator": "US 31"
                    },
                    "views": [
                        {
                            "uri": "electronic-sign/irisxsigns*81/2061651663",
                            "title": "US 31: US 31 SB Mile 131.2",
                            "category": "CMS",
                            "__typename": "SignTextView",
                            "textJustification": "CENTER",
                            "textLines": [
                                "MAIN   4 MI    4 MIN",
                                "116TH  6 MI    6 MIN",
                                "I-465  8 MI    8 MIN"
                            ],
                            "parentCollection": {
                                "uri": "electronic-sign/irisxsigns*81"
                            }
                        }
                    ]
                },
                {
                    "__typename": "Sign",
                    "uri": "electronic-sign/indianasigns*1652",
                    "title": "US 31: US 31 SB Mile 132.9",
                    "cityReference": "in Westfield",
                    "bbox": [
                        -86.13316,
                        40.05307,
                        -86.13316,
                        40.05307
                    ],
                    "icon": "/images/icon-traveltime-active-fill-solid.svg",
                    "color": "rgb(59,55,55)",
                    "signDisplayType": "OVERLAY_TRAVEL_TIME",
                    "signStatus": "DISPLAYING_MESSAGE",
                    "location": {
                        "primaryLinearReference": 116.74398994695434,
                        "secondaryLinearReference": 'null',
                        "routeDesignator": "US 31"
                    },
                    "views": [
                        {
                            "uri": "electronic-sign/indianasigns*1652/1671831259",
                            "title": "US 31: US 31 SB Mile 132.9",
                            "category": "OVERLAY_TRAVEL_TIME",
                            "__typename": "SignOverlayView",
                            "travelTimes": [
                                "4",
                                "9"
                            ],
                            "imageUrl": "/images/overlay-signs/1652.png",
                            "imageLayout": "ROW2_A",
                            "parentCollection": {
                                "uri": "electronic-sign/indianasigns*1652"
                            }
                        }
                    ]
                },
                {
                    "__typename": "Sign",
                    "uri": "electronic-sign/irisxsigns*192",
                    "title": "IN 37: IN 37 NB Mile 141.3",
                    "cityReference": "in Indianapolis",
                    "bbox": [
                        -86.2025,
                        39.64614,
                        -86.2025,
                        39.64614
                    ],
                    "icon": "/images/icon-sign-active-fill-solid.svg",
                    "color": "rgb(59,55,55)",
                    "signDisplayType": "CMS",
                    "signStatus": "DISPLAYING_MESSAGE",
                    "location": {
                        "primaryLinearReference": 114.84735881026744,
                        "secondaryLinearReference": 'null',
                        "routeDesignator": "IN 37"
                    },
                    "views": [
                        {
                            "uri": "electronic-sign/irisxsigns*192/1596412156",
                            "title": "IN 37: IN 37 NB Mile 141.3",
                            "category": "CMS",
                            "__typename": "SignTextView",
                            "textJustification": "CENTER",
                            "textLines": [
                                "I-465    4 MI  5 MIN",
                                "465E/65  8 MI  9 MIN",
                                "465W/70  9 MI 10 MIN"
                            ],
                            "parentCollection": {
                                "uri": "electronic-sign/irisxsigns*192"
                            }
                        }
                    ]
                },
                {
                    "__typename": "Sign",
                    "uri": "electronic-sign/irisxsigns*76",
                    "title": "IN 37: IN 37 SB Mile 172.2",
                    "cityReference": "in Noblesville",
                    "bbox": [
                        -86.00434,
                        40.00667,
                        -86.00434,
                        40.00667
                    ],
                    "icon": "/images/icon-sign-active-fill-solid.svg",
                    "color": "rgb(59,55,55)",
                    "signDisplayType": "CMS",
                    "signStatus": "DISPLAYING_MESSAGE",
                    "location": {
                        "primaryLinearReference": 122.54344519825715,
                        "secondaryLinearReference": 'null',
                        "routeDesignator": "IN 37"
                    },
                    "views": [
                        {
                            "uri": "electronic-sign/irisxsigns*76/3396903847",
                            "title": "IN 37: IN 37 SB Mile 172.2",
                            "category": "CMS",
                            "__typename": "SignTextView",
                            "textJustification": "CENTER",
                            "textLines": [
                                "116TH   3 MI   3 MIN",
                                "96TH    6 MI   6 MIN",
                                "I-465   8 MI   8 MIN"
                            ],
                            "parentCollection": {
                                "uri": "electronic-sign/irisxsigns*76"
                            }
                        }
                    ]
                },
                {
                    "__typename": "Sign",
                    "uri": "electronic-sign/irisxsigns*20",
                    "title": "I-64: I-64 EB Mile 120.4",
                    "cityReference": "1 mile west of New Albany",
                    "bbox": [
                        -85.87563,
                        38.30274,
                        -85.87563,
                        38.30274
                    ],
                    "icon": "/images/icon-sign-active-fill-solid.svg",
                    "color": "rgb(59,55,55)",
                    "signDisplayType": "CMS",
                    "signStatus": "DISPLAYING_MESSAGE",
                    "location": {
                        "primaryLinearReference": 120.29187383359586,
                        "secondaryLinearReference": 'null',
                        "routeDesignator": "I-64"
                    },
                    "views": [
                        {
                            "uri": "electronic-sign/irisxsigns*20/1330557060",
                            "title": "I-64: I-64 EB Mile 120.4",
                            "category": "CMS",
                            "__typename": "SignTextView",
                            "textJustification": "CENTER",
                            "textLines": [
                                "ROAD WORK I-64 E",
                                "NEAR OHIO RIVER BRDG",
                                "LEFT LANE CLOSED"
                            ],
                            "parentCollection": {
                                "uri": "electronic-sign/irisxsigns*20"
                            }
                        }
                    ]
                },
                {
                    "__typename": "Sign",
                    "uri": "electronic-sign/irisxsigns*21",
                    "title": "I-64: I-64 WB Mile 122.4",
                    "cityReference": "in New Albany",
                    "bbox": [
                        -85.84097,
                        38.29596,
                        -85.84097,
                        38.29596
                    ],
                    "icon": "/images/icon-sign-active-fill-solid.svg",
                    "color": "rgb(59,55,55)",
                    "signDisplayType": "CMS",
                    "signStatus": "DISPLAYING_MESSAGE",
                    "location": {
                        "primaryLinearReference": 122.2989123603719,
                        "secondaryLinearReference": 'null',
                        "routeDesignator": "I-64"
                    },
                    "views": [
                        {
                            "uri": "electronic-sign/irisxsigns*21/3028039813",
                            "title": "I-64: I-64 WB Mile 122.4",
                            "category": "CMS",
                            "__typename": "SignTextView",
                            "textJustification": "CENTER",
                            "textLines": [
                                "TRAVEL TIME TO",
                                "265/65  7 MI  8 MIN",
                                "SR135  17 MI 16 MIN"
                            ],
                            "parentCollection": {
                                "uri": "electronic-sign/irisxsigns*21"
                            }
                        }
                    ]
                },
                {
                    "__typename": "Sign",
                    "uri": "electronic-sign/irisxsigns*22",
                    "title": "I-65: I-65 SB Mile 0.8",
                    "cityReference": "in Jeffersonville",
                    "bbox": [
                        -85.75015,
                        38.27863,
                        -85.75015,
                        38.27863
                    ],
                    "icon": "/images/icon-sign-active-fill-solid.svg",
                    "color": "rgb(59,55,55)",
                    "signDisplayType": "CMS",
                    "signStatus": "DISPLAYING_MESSAGE",
                    "location": {
                        "primaryLinearReference": 0.8553079618352902,
                        "secondaryLinearReference": 'null',
                        "routeDesignator": "I-65"
                    },
                    "views": [
                        {
                            "uri": "electronic-sign/irisxsigns*22/323505926",
                            "title": "I-65: I-65 SB Mile 0.8",
                            "category": "CMS",
                            "__typename": "SignTextView",
                            "textJustification": "CENTER",
                            "textLines": [
                                "TO I-264 VIA",
                                "I-65   7 MI   7 MIN",
                                "I-64E  9 MI   8 MIN"
                            ],
                            "parentCollection": {
                                "uri": "electronic-sign/irisxsigns*22"
                            }
                        }
                    ]
                },
                {
                    "__typename": "Sign",
                    "uri": "electronic-sign/irisxsigns*24",
                    "title": "I-65: I-65 NB Mile 2.8",
                    "cityReference": "near Jeffersonville",
                    "bbox": [
                        -85.75237,
                        38.30435,
                        -85.75237,
                        38.30435
                    ],
                    "icon": "/images/icon-sign-active-fill-solid.svg",
                    "color": "rgb(59,55,55)",
                    "signDisplayType": "CMS",
                    "signStatus": "DISPLAYING_MESSAGE",
                    "location": {
                        "primaryLinearReference": 2.8129019345486204,
                        "secondaryLinearReference": 'null',
                        "routeDesignator": "I-65"
                    },
                    "views": [
                        {
                            "uri": "electronic-sign/irisxsigns*24/1403480960",
                            "title": "I-65: I-65 NB Mile 2.8",
                            "category": "CMS",
                            "__typename": "SignTextView",
                            "textJustification": "CENTER",
                            "textLines": [
                                "TRAF HAZARD I-265 W",
                                "AT GRANT LI/MILE 3",
                                "RAMP SLOW"
                            ],
                            "parentCollection": {
                                "uri": "electronic-sign/irisxsigns*24"
                            }
                        }
                    ]
                },
                {
                    "__typename": "Sign",
                    "uri": "electronic-sign/irisxsigns*23",
                    "title": "I-65: I-65 SB Mile 2.8",
                    "cityReference": "near Jeffersonville",
                    "bbox": [
                        -85.75274,
                        38.30455,
                        -85.75274,
                        38.30455
                    ],
                    "icon": "/images/icon-sign-active-fill-solid.svg",
                    "color": "rgb(59,55,55)",
                    "signDisplayType": "CMS",
                    "signStatus": "DISPLAYING_MESSAGE",
                    "location": {
                        "primaryLinearReference": 2.8255822975835296,
                        "secondaryLinearReference": 'null',
                        "routeDesignator": "I-65"
                    },
                    "views": [
                        {
                            "uri": "electronic-sign/irisxsigns*23/2020988679",
                            "title": "I-65: I-65 SB Mile 2.8",
                            "category": "CMS",
                            "__typename": "SignTextView",
                            "textJustification": "CENTER",
                            "textLines": [
                                "KY      3 MI   3 MIN",
                                "I-264   9 MI   9 MIN",
                                ""
                            ],
                            "parentCollection": {
                                "uri": "electronic-sign/irisxsigns*23"
                            }
                        }
                    ]
                },
                {
                    "__typename": "Sign",
                    "uri": "electronic-sign/irisxsigns*25",
                    "title": "I-65: I-65 SB Mile 7.9",
                    "cityReference": "in Sellersburg",
                    "bbox": [
                        -85.75797,
                        38.37752,
                        -85.75797,
                        38.37752
                    ],
                    "icon": "/images/icon-sign-active-fill-solid.svg",
                    "color": "rgb(59,55,55)",
                    "signDisplayType": "CMS",
                    "signStatus": "DISPLAYING_MESSAGE",
                    "location": {
                        "primaryLinearReference": 7.884767323243375,
                        "secondaryLinearReference": 'null',
                        "routeDesignator": "I-65"
                    },
                    "views": [
                        {
                            "uri": "electronic-sign/irisxsigns*25/3100963713",
                            "title": "I-65: I-65 SB Mile 7.9",
                            "category": "CMS",
                            "__typename": "SignTextView",
                            "textJustification": "CENTER",
                            "textLines": [
                                "TRAF HAZARD I-265 W",
                                "AT GRANT LI/MILE 3",
                                "RAMP SLOW"
                            ],
                            "parentCollection": {
                                "uri": "electronic-sign/irisxsigns*25"
                            }
                        }
                    ]
                },
                {
                    "__typename": "Sign",
                    "uri": "electronic-sign/irisxsigns*26",
                    "title": "I-65: I-65 NB Mile 8.1",
                    "cityReference": "in Sellersburg",
                    "bbox": [
                        -85.7588,
                        38.38136,
                        -85.7588,
                        38.38136
                    ],
                    "icon": "/images/icon-sign-active-fill-solid.svg",
                    "color": "rgb(59,55,55)",
                    "signDisplayType": "CMS",
                    "signStatus": "DISPLAYING_MESSAGE",
                    "location": {
                        "primaryLinearReference": 8.152994761344617,
                        "secondaryLinearReference": 'null',
                        "routeDesignator": "I-65"
                    },
                    "views": [
                        {
                            "uri": "electronic-sign/irisxsigns*26/396429826",
                            "title": "I-65: I-65 NB Mile 8.1",
                            "category": "CMS",
                            "__typename": "SignTextView",
                            "textJustification": "CENTER",
                            "textLines": [
                                "SR56  21 MI   18 MIN",
                                "US50  41 MI   35 MIN",
                                "I-465 98 MI   85 MIN"
                            ],
                            "parentCollection": {
                                "uri": "electronic-sign/irisxsigns*26"
                            }
                        }
                    ]
                },
                {
                    "__typename": "Sign",
                    "uri": "electronic-sign/irisxsigns*183",
                    "title": "I-65: I-65 NB Mile 47.2",
                    "cityReference": "2 miles south of Seymour",
                    "bbox": [
                        -85.83758,
                        38.92484,
                        -85.83758,
                        38.92484
                    ],
                    "icon": "/images/icon-sign-active-fill-solid.svg",
                    "color": "rgb(59,55,55)",
                    "signDisplayType": "CMS",
                    "signStatus": "DISPLAYING_MESSAGE",
                    "location": {
                        "primaryLinearReference": 47.16345335018031,
                        "secondaryLinearReference": 'null',
                        "routeDesignator": "I-65"
                    },
                    "views": [
                        {
                            "uri": "electronic-sign/irisxsigns*183/3328487836",
                            "title": "I-65: I-65 NB Mile 47.2",
                            "category": "CMS",
                            "__typename": "SignTextView",
                            "textJustification": "CENTER",
                            "textLines": [
                                "SR-58  16 MI  14 MIN",
                                "US-31  28 MI  24 MIN",
                                "I-465  59 MI  50 MIN"
                            ],
                            "parentCollection": {
                                "uri": "electronic-sign/irisxsigns*183"
                            }
                        }
                    ]
                },
                {
                    "__typename": "Sign",
                    "uri": "electronic-sign/irisxsigns*194",
                    "title": "I-65: I-65 NB Mile 65.4",
                    "cityReference": "in Columbus",
                    "bbox": [
                        -85.95802,
                        39.15917,
                        -85.95802,
                        39.15917
                    ],
                    "icon": "/images/icon-sign-active-fill-solid.svg",
                    "color": "rgb(59,55,55)",
                    "signDisplayType": "CMS",
                    "signStatus": "DISPLAYING_MESSAGE",
                    "location": {
                        "primaryLinearReference": 65.41669438275471,
                        "secondaryLinearReference": 'null',
                        "routeDesignator": "I-65"
                    },
                    "views": [
                        {
                            "uri": "electronic-sign/irisxsigns*194/525142842",
                            "title": "I-65: I-65 NB Mile 65.4",
                            "category": "CMS",
                            "__typename": "SignTextView",
                            "textJustification": "CENTER",
                            "textLines": [
                                "US-31  10 MI   9 MIN",
                                "SR-44  24 MI  21 MIN",
                                "I-465  41 MI  35 MIN"
                            ],
                            "parentCollection": {
                                "uri": "electronic-sign/irisxsigns*194"
                            }
                        }
                    ]
                },
                {
                    "__typename": "Sign",
                    "uri": "electronic-sign/irisxsigns*195",
                    "title": "I-65: I-65 SB Mile 65.4",
                    "cityReference": "in Columbus",
                    "bbox": [
                        -85.95854,
                        39.15924,
                        -85.95854,
                        39.15924
                    ],
                    "icon": "/images/icon-sign-active-fill-solid.svg",
                    "color": "rgb(59,55,55)",
                    "signDisplayType": "CMS",
                    "signStatus": "DISPLAYING_MESSAGE",
                    "location": {
                        "primaryLinearReference": 65.42151517570743,
                        "secondaryLinearReference": 'null',
                        "routeDesignator": "I-65"
                    },
                    "views": [
                        {
                            "uri": "electronic-sign/irisxsigns*195/801511963",
                            "title": "I-65: I-65 SB Mile 65.4",
                            "category": "CMS",
                            "__typename": "SignTextView",
                            "textJustification": "CENTER",
                            "textLines": [
                                "ROAD WORK I-65 S",
                                "NEAR SR 46 / MILE 68",
                                "LEFT LANE CLOSED"
                            ],
                            "parentCollection": {
                                "uri": "electronic-sign/irisxsigns*195"
                            }
                        }
                    ]
                },
                {
                    "__typename": "Sign",
                    "uri": "electronic-sign/irisxsigns*29",
                    "title": "I-65: I-65 NB Mile 86.5",
                    "cityReference": "2 miles south of Franklin",
                    "bbox": [
                        -85.9876,
                        39.44232,
                        -85.9876,
                        39.44232
                    ],
                    "icon": "/images/icon-sign-active-fill-solid.svg",
                    "color": "rgb(59,55,55)",
                    "signDisplayType": "CMS",
                    "signStatus": "DISPLAYING_MESSAGE",
                    "location": {
                        "primaryLinearReference": 86.43497719390008,
                        "secondaryLinearReference": 'null',
                        "routeDesignator": "I-65"
                    },
                    "views": [
                        {
                            "uri": "electronic-sign/irisxsigns*29/1391310989",
                            "title": "I-65: I-65 NB Mile 86.5",
                            "category": "CMS",
                            "__typename": "SignTextView",
                            "textJustification": "CENTER",
                            "textLines": [
                                "CO LN 14 MI  12 MIN",
                                "I-465 20 MI  18 MIN",
                                "I-70  24 MI  22 MIN"
                            ],
                            "parentCollection": {
                                "uri": "electronic-sign/irisxsigns*29"
                            }
                        }
                    ]
                },
                {
                    "__typename": "Sign",
                    "uri": "electronic-sign/indianasigns*1542",
                    "title": "I-65: I-65 NB Mile 100.0",
                    "cityReference": "in Greenwood",
                    "bbox": [
                        -86.07389,
                        39.62624,
                        -86.07389,
                        39.62624
                    ],
                    "icon": "/images/icon-traveltime-active-fill-solid.svg",
                    "color": "rgb(59,55,55)",
                    "signDisplayType": "OVERLAY_TRAVEL_TIME",
                    "signStatus": "DISPLAYING_MESSAGE",
                    "location": {
                        "primaryLinearReference": 100.07767150741782,
                        "secondaryLinearReference": 'null',
                        "routeDesignator": "I-65"
                    },
                    "views": [
                        {
                            "uri": "electronic-sign/indianasigns*1542/3903539513",
                            "title": "I-65: I-65 NB Mile 100.0",
                            "category": "OVERLAY_TRAVEL_TIME",
                            "__typename": "SignOverlayView",
                            "travelTimes": [
                                "6",
                                "10"
                            ],
                            "imageUrl": "/images/overlay-signs/1542.png",
                            "imageLayout": "ROW2_A",
                            "parentCollection": {
                                "uri": "electronic-sign/indianasigns*1542"
                            }
                        }
                    ]
                },
                {
                    "__typename": "Sign",
                    "uri": "electronic-sign/indianasigns*1578",
                    "title": "I-65: I-65 NB Mile 101.8",
                    "cityReference": "in Indianapolis",
                    "bbox": [
                        -86.07397,
                        39.6511,
                        -86.07397,
                        39.6511
                    ],
                    "icon": "/images/icon-traveltime-active-fill-solid.svg",
                    "color": "rgb(59,55,55)",
                    "signDisplayType": "OVERLAY_TRAVEL_TIME",
                    "signStatus": "DISPLAYING_MESSAGE",
                    "location": {
                        "primaryLinearReference": 101.7923680493749,
                        "secondaryLinearReference": 'null',
                        "routeDesignator": "I-65"
                    },
                    "views": [
                        {
                            "uri": "electronic-sign/indianasigns*1578/1393186000",
                            "title": "I-65: I-65 NB Mile 101.8",
                            "category": "OVERLAY_TRAVEL_TIME",
                            "__typename": "SignOverlayView",
                            "travelTimes": [
                                "21",
                                "24"
                            ],
                            "imageUrl": "/images/overlay-signs/1578.png",
                            "imageLayout": "ROW2_A",
                            "parentCollection": {
                                "uri": "electronic-sign/indianasigns*1578"
                            }
                        }
                    ]
                },
                {
                    "__typename": "Sign",
                    "uri": "electronic-sign/irisxsigns*30",
                    "title": "I-65: I-65 SB Mile 104.4",
                    "cityReference": "in Indianapolis",
                    "bbox": [
                        -86.10156,
                        39.68051,
                        -86.10156,
                        39.68051
                    ],
                    "icon": "/images/icon-sign-active-fill-solid.svg",
                    "color": "rgb(59,55,55)",
                    "signDisplayType": "CMS",
                    "signStatus": "DISPLAYING_MESSAGE",
                    "location": {
                        "primaryLinearReference": 104.38181032254096,
                        "secondaryLinearReference": 'null',
                        "routeDesignator": "I-65"
                    },
                    "views": [
                        {
                            "uri": "electronic-sign/irisxsigns*30/712553381",
                            "title": "I-65: I-65 SB Mile 104.4",
                            "category": "CMS",
                            "__typename": "SignTextView",
                            "textJustification": "CENTER",
                            "textLines": [
                                "ROAD WORK I-65 S",
                                "NEAR SR 46 / MILE 68",
                                "LEFT LANE CLOSED"
                            ],
                            "parentCollection": {
                                "uri": "electronic-sign/irisxsigns*30"
                            }
                        }
                    ]
                },
                {
                    "__typename": "Sign",
                    "uri": "electronic-sign/irisxsigns*31",
                    "title": "I-65: I-65 NB Mile 104.7",
                    "cityReference": "in Indianapolis",
                    "bbox": [
                        -86.10245,
                        39.68452,
                        -86.10245,
                        39.68452
                    ],
                    "icon": "/images/icon-sign-active-fill-solid.svg",
                    "color": "rgb(59,55,55)",
                    "signDisplayType": "CMS",
                    "signStatus": "DISPLAYING_MESSAGE",
                    "location": {
                        "primaryLinearReference": 104.66153203723593,
                        "secondaryLinearReference": 'null',
                        "routeDesignator": "I-65"
                    },
                    "views": [
                        {
                            "uri": "electronic-sign/irisxsigns*31/3045512164",
                            "title": "I-65: I-65 NB Mile 104.7",
                            "category": "CMS",
                            "__typename": "SignTextView",
                            "textJustification": "CENTER",
                            "textLines": [
                                "65 THRU TRAFFIC VIA",
                                "I-65 18 MI   20 MIN",
                                "465W 21 MI   23 MIN"
                            ],
                            "parentCollection": {
                                "uri": "electronic-sign/irisxsigns*31"
                            }
                        }
                    ]
                },
                {
                    "__typename": "Sign",
                    "uri": "electronic-sign/indianasigns*1699",
                    "title": "I-65: I-65 SB Mile 104.7",
                    "cityReference": "in Indianapolis",
                    "bbox": [
                        -86.103,
                        39.68455,
                        -86.103,
                        39.68455
                    ],
                    "icon": "/images/icon-traveltime-active-fill-solid.svg",
                    "color": "rgb(59,55,55)",
                    "signDisplayType": "OVERLAY_TRAVEL_TIME",
                    "signStatus": "DISPLAYING_MESSAGE",
                    "location": {
                        "primaryLinearReference": 104.67050921905242,
                        "secondaryLinearReference": 'null',
                        "routeDesignator": "I-65"
                    },
                    "views": [
                        {
                            "uri": "electronic-sign/indianasigns*1699/93202012",
                            "title": "I-65: I-65 SB Mile 104.7",
                            "category": "OVERLAY_TRAVEL_TIME",
                            "__typename": "SignOverlayView",
                            "travelTimes": [
                                "5",
                                "14"
                            ],
                            "imageUrl": "/images/overlay-signs/1699.png",
                            "imageLayout": "ROW2_A",
                            "parentCollection": {
                                "uri": "electronic-sign/indianasigns*1699"
                            }
                        }
                    ]
                },
                {
                    "__typename": "Sign",
                    "uri": "electronic-sign/indianasigns*1582",
                    "title": "I-65: I-65 NB Mile 107.2",
                    "cityReference": "in Indianapolis",
                    "bbox": [
                        -86.12132,
                        39.71618,
                        -86.12132,
                        39.71618
                    ],
                    "icon": "/images/icon-traveltime-active-fill-solid.svg",
                    "color": "rgb(59,55,55)",
                    "signDisplayType": "OVERLAY_TRAVEL_TIME",
                    "signStatus": "DISPLAYING_MESSAGE",
                    "location": {
                        "primaryLinearReference": 107.21947597080812,
                        "secondaryLinearReference": 'null',
                        "routeDesignator": "I-65"
                    },
                    "views": [
                        {
                            "uri": "electronic-sign/indianasigns*1582/3146078389",
                            "title": "I-65: I-65 NB Mile 107.2",
                            "category": "OVERLAY_TRAVEL_TIME",
                            "__typename": "SignOverlayView",
                            "travelTimes": [
                                "3",
                                "16"
                            ],
                            "imageUrl": "/images/overlay-signs/1582.png",
                            "imageLayout": "ROW2_A",
                            "parentCollection": {
                                "uri": "electronic-sign/indianasigns*1582"
                            }
                        }
                    ]
                }
            ],
            "totalRecords": 148,
            "error": 'null'
        }
    }
}


# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, rc):
    print("Connected with result code "+str(rc))
    # Subscribing in on_connect() means that if we lose the connection and
    # reconnect then subscriptions will be renewed.
    client.subscribe("V2X-Traveller-Information")

# The callback for when a PUBLISH message is received from the server.


def on_message(client, userdata, msg):
    print(msg.topic+" "+str(msg.payload))


pub_ts_map = {}
pub_messages = 0


def on_publish(client, userdata, mid):
    global pub_messages
    pub_messages += 1
    print("Published: " + str(mid))


class GPSThread(threading.Thread):
    def __init__(self):
        super(GPSThread, self).__init__()
        self.lat = None
        self.lon = None
        self.time = None
        self.alt = None
        self.speed = None

    def run(self):
        with GPSDClient() as client:
            for result in client.dict_stream(convert_datetime=True, filter=["TPV"]):
                self.lat = result.get("lat", "n/a")
                self.lon = result.get("lon", "n/a")
                self.time = result.get("time", "n/a")
                self.alt = result.get("alt", "n/a")
                self.speed = result.get("speed", "n/a")


# Example usage
def main():
    # Create and start the GPS thread
    gps_thread = GPSThread()
    gps_thread.start()

    client = mqtt.Client()
    client.on_connect = on_connect
    client.on_message = on_message
    client.on_publish = on_publish

    client.connect("localhost", 1883, 60)
    # client.connect("10.0.2.12", 1883, 60)

    signs_list = []

    signs = orgResponse["data"]["listSignsQuery"]["signs"]

    for sign in signs:
        sign_json = {
            "time": time.time(),
            "Latitude":  gps_thread.lat,
            "Longitude":  gps_thread.lon,
            "GPSTime":  str(gps_thread.time),
            "Altitude":   gps_thread.alt,
            "Speed":   gps_thread.speed,
            "__typename": sign["__typename"],
            "uri": sign["uri"],
            "title": sign["title"],
            "cityReference": sign["cityReference"],
            "bbox": sign["bbox"],
            "icon": sign["icon"],
            "color": sign["color"],
            "signDisplayType": sign["signDisplayType"],
            "signStatus": sign["signStatus"],
            "location": sign["location"],
            "views": sign["views"]
        }

        signs_list.append(sign_json)
    while True:
        # Print the extracted sign JSONs
        for sign in signs_list:
            print(sign)
            # Serialize and encode the message
            MESSAGE = json.dumps(sign).encode()
            client.publish("V2X-Traveller-Information", MESSAGE)
            time.sleep(1)

    client.disconnect()
    print("done")


if __name__ == "__main__":
    main()
